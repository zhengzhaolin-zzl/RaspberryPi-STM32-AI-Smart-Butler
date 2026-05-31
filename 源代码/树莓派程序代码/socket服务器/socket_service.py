import socket
import os
import dashscope
messages = [
    {'role': 'system', 'content': 'You are a helpful assistant.'},
    {'role': 'user', 'content': '你能会我提供什么服务？'}
]
ip = ''
port = 8080
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server_socket.bind((ip, port))
server_socket.listen(5)
print('Waiting for a connection...')
#等待连接
user_socket,user_addr = server_socket.accept() #阻塞
print('用户:'+str(user_addr[0])+'连接成功')
data = ''
while data != 'byebye':
    data = user_socket.recv(1024).decode('gbk')
    print(data,type(data))
    if data[0:2] == "aa":
        print('here')
        messages[1]["content"] = data
        response = dashscope.Generation.call(
            api_key="sk-a5a3c5c06d914bbc951b8cb6a3868792",
            model="qwen-max",  
            messages=messages,
            result_format='message'
        )
         # 在回答后面加上 "this is zzl"，自定义数据包格式
        answer = response["output"]["choices"][0]["message"]["content"] + " this is zzl"
        print(answer)
        user_socket.send(answer.encode('utf-8'))
server_socket.close()
