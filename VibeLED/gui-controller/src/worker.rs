use crate::audio::player::Player;
use crate::audio::scanner;
use crate::config::Config;
use crate::esp::{broadcast, scene};
use crate::state::{AppEvent, Cmd, Track};
use std::path::Path;
use tokio::sync::mpsc::{Receiver, Sender};

pub async fn run(
    mut cmd_rx: Receiver<Cmd>,
    event_tx: Sender<AppEvent>,
    mut config: Config,
) {
    let mut player = match Player::new() {
        Ok(p) => p,
        Err(e) => {
            let _ = event_tx.send(AppEvent::Error(format!("Audio Init Error: {}", e))).await;
            return;
        }
    };
    let mut tracks: Vec<Track> = vec![];

    // Initial track scan
    tracks = scanner::scan(Path::new(&config.music_folder));
    let _ = event_tx.send(AppEvent::TracksLoaded(tracks.clone())).await;

    while let Some(cmd) = cmd_rx.recv().await {
        match cmd {
            Cmd::Play(idx) => {
                if let Some(track) = tracks.get(idx) {
                    match player.play(&track.path) {
                        Ok(_) => {
                            let _ = event_tx.send(AppEvent::PlaybackStarted(track.name.clone())).await;
                        }
                        Err(e) => {
                            let _ = event_tx.send(AppEvent::Error(e)).await;
                        }
                    }
                }
            }
            Cmd::Pause => {
                player.pause();
            }
            Cmd::Stop => {
                player.stop();
                let _ = event_tx.send(AppEvent::PlaybackStopped).await;
            }

            Cmd::CriticalStop { total, required } => {
                player.stop();
                let signals = scene::random_assign(total, required);
                let _ = broadcast::send_signals(&signals, &config.broadcast_ip, config.udp_port).await;
                let _ = event_tx.send(AppEvent::EspFired(signals)).await;
                let _ = event_tx.send(AppEvent::PlaybackStopped).await;
            }

            Cmd::RefreshTracks => {
                tracks = scanner::scan(Path::new(&config.music_folder));
                let _ = event_tx.send(AppEvent::TracksLoaded(tracks.clone())).await;
            }

            Cmd::SetTotalEsps(total) => {
                config.total_esps = total;
                config.save();
            }
            
            Cmd::SetRequiredEsps(req) => {
                config.required_esps = req;
                config.save();
            }

            Cmd::ChangeMusicFolder(folder) => {
                config.music_folder = folder;
                config.save();
                tracks = scanner::scan(Path::new(&config.music_folder));
                let _ = event_tx.send(AppEvent::TracksLoaded(tracks.clone())).await;
            }

            Cmd::AddTracks(paths) => {
                let mut new_tracks = vec![];
                let supp = ["mp3", "ogg", "flac", "wav", "m4a"];
                for path in paths {
                    if path.is_file() {
                        if let Some(ext) = path.extension().and_then(|e| e.to_str()) {
                            if supp.contains(&ext.to_lowercase().as_str()) {
                                new_tracks.push(Track {
                                    name: path.file_name().unwrap_or_default().to_string_lossy().into_owned(),
                                    path,
                                });
                            }
                        }
                    }
                }
                if !new_tracks.is_empty() {
                    tracks.extend(new_tracks);
                    let _ = event_tx.send(AppEvent::TracksLoaded(tracks.clone())).await;
                }
            }

            _ => {}
        }
    }
}
