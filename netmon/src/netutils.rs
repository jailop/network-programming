use std::collections::HashMap;
use std::collections::VecDeque;
use std::{thread, time};
use sysinfo::Networks;

type TimePoint = (u64, u64);
type IfacePoint = HashMap<String, TimePoint>;

fn network_traffic() -> IfacePoint {
    let networks = Networks::new_with_refreshed_list();
    let mut res = HashMap::<String, TimePoint>::new();
    for (iface, network) in &networks {
        res.insert(
            iface.to_string(),
            (network.total_received(), network.total_transmitted())
        );
    }
    res
}

fn relative_traffic_by_iface(iface: &str, dataset: &VecDeque<IfacePoint>)
        -> Vec<TimePoint> {
    dataset
        .iter()
        .map(|data| data[iface])
        .collect::<Vec<_>>()
        .windows(2)
        .map(|w| (w[1].0 - w[0].0, w[1].1 - w[0].1))
        .collect::<Vec<_>>()
}

pub fn network_queue() {
    const BUFSIZE: usize = 60;
    let mut queue = VecDeque::<IfacePoint>::with_capacity(60);
    loop {
        let values = network_traffic();
        queue.push_back(values);
        if queue.len() > BUFSIZE {
            queue.pop_front();
        }
        let points = relative_traffic_by_iface("wlan0", &queue);
        println!("{:?}", points);
        thread::sleep(time::Duration::from_millis(1000));
    }
}
