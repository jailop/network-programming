use std::collections::HashSet;
use tokio::net::{TcpListener, TcpStream};
use std::sync::{Mutex, Arc};
use tokio_tungstenite::{WebSocketStream, accept_async};

type WebSocket = WebSocketStream<TcpStream>;

pub async fn run_server() {
    let addr = "127.0.0.1:28000";
    let listener = TcpListener::bind(addr).await.unwrap();
    let clients = Arc::new(Mutex::new(HashSet::<WebSocket>::new()));
    while let Ok((stream, _)) = listener.accept().await {
        let cloned_clients = Arc::clone(&clients);
        handle_connection(stream, cloned_clients);
    }
}

async fn handle_connection(stream: TcpStream, clients: Arc<Mutex<HashSet<WebSocket>>>) {
    if let Ok(ws_stream) = accept_async(stream).await {
        let cloned_clients = Arc::clone(&clients);
        // cloned_clients.lock().await.insert(ws_stream.clone());
        // let (write, read) = ws_stream.split();
    }
}
