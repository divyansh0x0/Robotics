use crate::state::EspSignal;
use tokio::net::UdpSocket;
use std::time::Duration;

pub async fn send_signals(
    signals: &[EspSignal],
    broadcast_ip: &str,
    port: u16,
) -> Result<(), String> {
    let addr = format!("{}:{}", broadcast_ip, port);
    let sock = UdpSocket::bind("0.0.0.0:0").await.map_err(|e| e.to_string())?;
    sock.set_broadcast(true).map_err(|e| e.to_string())?;

    for signal in signals {
        // Two big-endian f32 values = 8 bytes
        let mut buf = [0u8; 8];
        // Cast id to f32 before serialising as ESP parses it as f32
        buf[0..4].copy_from_slice(&(signal.id as f32).to_bits().to_be_bytes());
        buf[4..8].copy_from_slice(&signal.state.to_bits().to_be_bytes());

        sock.send_to(&buf, &addr).await.map_err(|e| e.to_string())?;

        // Small delay between packets to avoid overwhelming ESP's UDP buffer
        tokio::time::sleep(Duration::from_millis(10)).await;
    }

    Ok(())
}
