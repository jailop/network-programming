pub mod netutils;
pub mod websock;

use netutils::network_queue;

fn main() {
    network_queue();
}

