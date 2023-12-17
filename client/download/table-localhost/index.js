HTTP/1.1 200 OK
Host: localhost:8081
Date: Thu, 14 Dec 2023 21:18:08 GMT
Connection: close
Content-Type: application/javascript
Content-Length: 239

window.onload = async () => {
    thisApp = await tabgram.registerApp({name : 'table'})
    
    tabgramPipe.onInput((data) => {
        console.log(data);
        const globalData = data;
        tabgramPipe.output(globalData);
    })
}

