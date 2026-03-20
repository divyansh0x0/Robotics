use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize, Clone)]
pub struct Config {
    pub music_folder:  String,          // absolute path to music directory
    pub udp_port:      u16,             // ESP listening port (default: 8080)
    pub broadcast_ip:  String,          // default: "255.255.255.255"
    pub total_esps:    u32,             // total number of ESPs on network
    pub required_esps: u32,             // how many must be ON after critical stop
}

impl Default for Config {
    fn default() -> Self {
        Self {
            music_folder:  "./music".into(),
            udp_port:      8080,
            broadcast_ip:  "255.255.255.255".into(),
            total_esps:    10,
            required_esps: 3,
        }
    }
}

impl Config {
    pub fn load() -> Self {
        let path = "config.json";
        if let Ok(data) = std::fs::read_to_string(path) {
            if let Ok(config) = serde_json::from_str(&data) {
                return config;
            }
        }
        Self::default()
    }

    pub fn save(&self) {
        if let Ok(data) = serde_json::to_string_pretty(self) {
            let _ = std::fs::write("config.json", data);
        }
    }
}
