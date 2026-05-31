import asyncio
from contextlib import AsyncExitStack
import json
from dotenv import load_dotenv
import os
from typing import Optional
from mcp import ClientSession, StdioServerParameters
from mcp.client.stdio import stdio_client
from mcp.client.sse import sse_client
from openai import AsyncOpenAI
import azure.cognitiveservices.speech as speechsdk
import time

load_dotenv("AI.env")

# 加载配置
azure_speech_key = os.environ["Azure_speech_key"]
azure_speech_region = os.environ["Azure_speech_region"]
azure_speech_speaker = os.environ["Azure_speech_speaker"]
wakeup_word = os.environ["WakeupWord"]
wakeup_model_file = os.environ["WakeupModelFile"]

messages = []

# 初始化 Azure 语音服务
speech_config = speechsdk.SpeechConfig(subscription=azure_speech_key, region=azure_speech_region)
speech_config.speech_synthesis_language = "zh-CN"
speech_config.speech_recognition_language = "zh-CN"
speech_config.speech_synthesis_voice_name = azure_speech_speaker

# 文本到语音合成器
speech_synthesizer = speechsdk.SpeechSynthesizer(speech_config=speech_config)
connection = speechsdk.Connection.from_speech_synthesizer(speech_synthesizer)
connection.open(True)

# 语音识别相关配置
model = speechsdk.KeywordRecognitionModel(wakeup_model_file)
audio_config = speechsdk.audio.AudioConfig(use_default_microphone=True)
auto_detect_source_language_config = speechsdk.languageconfig.AutoDetectSourceLanguageConfig(languages=["ja-JP", "zh-CN"])
speech_recognizer = speechsdk.SpeechRecognizer(
    speech_config=speech_config,
    audio_config=audio_config,
    auto_detect_source_language_config=auto_detect_source_language_config
)

unknown_count = 0
lang = "zh-CN"
sys_message = {"role": "system", "content": os.environ["sysprompt_zh-CN"]}
tts_end_marks = [".", "!", "?", ";", "。", "！", "？", "；", "\n"]

is_listening = False

def show_text(s):
    print(s)

def recognize_speech():
    global unknown_count, is_listening
    result = speech_recognizer.recognize_once_async().get()
    if result.reason == speechsdk.ResultReason.RecognizedSpeech:
        unknown_count = 0
        is_listening = False
        return result.text
    elif result.reason == speechsdk.ResultReason.NoMatch:
        is_listening = False
        unknown_count += 1
        return '...'
    elif result.reason == speechsdk.ResultReason.Canceled:
        is_listening = False
        return "语音识别已取消。"

def synthesize_speech(text):
    try:
        # 同步语音合成
        result = speech_synthesizer.speak_text_async(text).get()
        return result.reason == speechsdk.ResultReason.SynthesizingAudioCompleted
    except Exception as ex:
        print(f"语音合成错误: {ex}")
        return False

model_name = os.environ["model"]

class VoiceAssistant:
    def __init__(self):
        self.session = None
        self.sessions = {}
        self.exit_stack = AsyncExitStack()
        self.tools = []
        self.messages = []
        self.client = AsyncOpenAI(
            api_key=os.environ["key"],
            base_url=os.environ["base_url"]
        )

    async def cleanup_resources(self):
        await self.exit_stack.aclose()

    async def connect_to_server(self):
        with open("mcp_server_config.json", "r") as f:
            config = json.load(f)
        servers = config["mcpServers"]
        for server_key in servers.keys():
            server_config = servers[server_key]
            session = None
            if "url" in server_config and server_config["isActive"] and "type" in server_config and server_config["type"] == "sse":
                server_url = server_config["url"]
                sse_transport = await self.exit_stack.enter_async_context(sse_client(server_url))
                write, read = sse_transport
                session = await self.exit_stack.enter_async_context(ClientSession(write, read))
            elif "command" in server_config and server_config["isActive"]:
                command = server_config["command"]
                args = server_config["args"]
                server_params = StdioServerParameters(command=command, args=args, env=None)
                stdio_transport = await self.exit_stack.enter_async_context(stdio_client(server_params))
                stdio_in, stdio_out = stdio_transport
                session = await self.exit_stack.enter_async_context(ClientSession(stdio_in, stdio_out))
            if session:
                await session.initialize()
                response = await session.list_tools()
                tools = response.tools
                for tool in tools:
                    self.sessions[tool.name] = session
                self.tools += tools
        print("服务初始化完成！")

    async def process_request(self, query: str) -> str:
        self.messages.append({"role": "user", "content": query})
        recent_messages = self.messages[-10:]

        available_tools = [{
            "type": "function",
            "function": {
                "name": tool.name,
                "description": tool.description,
                "parameters": tool.inputSchema
            }
        } for tool in self.tools]

        extra_body = {
            "enable_thinking": False
        }

        response_gen = await self.client.chat.completions.create(
            model=model_name,
            messages=[sys_message] + recent_messages,
            tools=available_tools,
            stream=True,
            extra_body=extra_body
        )

        final_text = []
        function_list = []
        collected_messages = []
        split = True

        async for chunk in response_gen:
            if chunk:
                delta = chunk.choices[0].delta
                chunk_message = delta.content

                if chunk_message is not None and chunk_message != '':
                    collected_messages.append(chunk_message)
                    final_text.append(chunk_message)
                    if chunk_message in tts_end_marks and split:
                        text = ''.join(collected_messages).strip()
                        if len(text) > 500 or "</think>" in text:
                            split = False
                        if text != '':
                            print(f"Speech synthesized to speaker for: {text}")
                            synthesize_speech(text)
                            collected_messages.clear()

                if delta.tool_calls:
                    for tool_call in delta.tool_calls:
                        if len(function_list) < tool_call.index + 1:
                            function_list.append({'name': '', 'args': '', 'id': tool_call.id})
                        if tool_call and tool_call.function.name:
                            function_list[tool_call.index]['name'] += tool_call.function.name
                        if tool_call and tool_call.function.arguments:
                            function_list[tool_call.index]['args'] += tool_call.function.arguments

        if len(function_list) > 0:
            findex = 0
            tool_calls = []
            temp_messages = []
            for func in function_list:
                function_name = func["name"]
                function_args = func["args"]
                toolid = func["id"]
                if function_name != '':
                    tool_name = function_name
                    tool_args = json.loads(function_args)

                    function_response = await self.sessions[tool_name].call_tool(tool_name, tool_args)
                    print(f"MCP: [Calling tool {tool_name} with args {tool_args}]")
                    print(f'⏳result: {function_response}')

                    tool_calls.append({"id": toolid, "function": {"arguments": func["args"], "name": function_name}, "type": "function", "index": findex})

                    temp_messages.append(
                        {
                            "tool_call_id": toolid,
                            "role": "tool",
                            "name": function_name,
                            "content": function_response.content[0].text,
                        }
                    )

                    findex += 1

            recent_messages.append({
                "role": "assistant",
                "content": '',
                "tool_calls": tool_calls,
            })

            for m in temp_messages:
                recent_messages.append(m)

            return await self.process_request("继续")
        else:
            if len(collected_messages) > 0:
                text = ''.join(collected_messages).strip()
                if text != '':
                    print(f"Speech synthesized to speaker for: {text}")
                    synthesize_speech(text)

            response_text = ''.join(final_text)
            self.messages.append({"role": "assistant", "content": response_text})
            return response_text

    async def check_player_status(self):
        result = await self.sessions["isPlaying"].call_tool("isPlaying", {})
        return "playing" if result.content[0].text == "true" else ""

    async def interaction_loop(self):
        print("语音助手已启动！")
        global unknown_count

        while True:
            # 唤醒词检测 (每次创建新实例)
            keyword_recognizer = speechsdk.KeywordRecognizer()
            try:
                print("等待唤醒...")
                result_future = keyword_recognizer.recognize_once_async(model)
                result = result_future.get()

                if result.reason == speechsdk.ResultReason.RecognizedKeyword:
                    print(f"检测到唤醒词: {result.text}")
                    synthesize_speech("您好，请说出您的需求")

                    # 对话循环
                    while unknown_count < 3:
                        user_input = recognize_speech()
                        if user_input in ["退出", "结束"]:
                            break
                        if user_input == "...":
                            continue

                        response = await self.process_request(user_input)

                    # 退出流程
                    goodbye_message = "我先退下了，请随时唤醒我说你好希希"
                    print(goodbye_message)
                    synthesize_speech(goodbye_message)

                    # 立即重置状态并休眠
                    unknown_count = 0
                    break  # 跳出当前循环重新初始化

            except Exception as e:
                print(f"运行时异常: {str(e)}")
            finally:
                # 正确释放资源
                del keyword_recognizer
                time.sleep(0.5)  # 确保资源释放

async def main():
    assistant = VoiceAssistant()
    try:
        await assistant.connect_to_server()
        while True:  # 外层循环确保持续运行
            await assistant.interaction_loop()
    finally:
        await assistant.cleanup_resources()

if __name__ == "__main__":
    asyncio.run(main())