# MY CHAT PROTOCOL

### Compiling server & clients

- Compile server with:

 ```gcc -g -o server server.c app_layer/chat.c utils.c -lpthread ```

- Compile client with:

 ```gcc -g -o client client.c app_layer/chat.c utils.c ```
 
### Run with:

 ```./server```
 ```./client```
 
