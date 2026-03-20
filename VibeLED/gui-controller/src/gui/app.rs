use crate::state::{AppEvent, Cmd, UiState};
use crate::config::Config;
use crate::gui::panels::{esp_config, esp_grid, top_bar, track_list, playback_bar};
use std::time::Duration;
use tokio::sync::mpsc::{Receiver, Sender};

pub struct ControllerApp {
    cmd_tx: Sender<Cmd>,
    event_rx: Receiver<AppEvent>,
    state: UiState,
}

impl ControllerApp {
    pub fn new(cmd_tx: Sender<Cmd>, event_rx: Receiver<AppEvent>, config: Config) -> Self {
        let mut state = UiState::default();
        state.total_esps = config.total_esps;
        state.required_esps = config.required_esps;

        Self {
            cmd_tx,
            event_rx,
            state,
        }
    }

    fn drain_events(&mut self) {
        while let Ok(ev) = self.event_rx.try_recv() {
            match ev {
                AppEvent::TracksLoaded(t) => self.state.tracks = t,
                AppEvent::PlaybackStarted(idx, n) => {
                    self.state.playing = true;
                    self.state.current_track = Some(idx);
                    self.state.status_text = format!("Playing: {}", n);
                }
                AppEvent::PlaybackStopped => {
                    self.state.playing = false;
                    self.state.status_text = "Stopped".into();
                }
                AppEvent::PlaybackTick { position_ms, duration_ms } => {
                    self.state.playback_pos_ms = position_ms;
                    self.state.playback_dur_ms = duration_ms;
                }
                AppEvent::EspFired(sigs) => self.state.last_signals = sigs,
                AppEvent::Error(e) => self.state.error = Some(e),
            }
        }
    }
}

impl eframe::App for ControllerApp {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        self.drain_events();

        egui::TopBottomPanel::top("top_bar")
            .exact_height(60.0)
            .show(ctx, |ui| top_bar::show(ui, &self.state, &self.cmd_tx));

        egui::TopBottomPanel::bottom("playback_bar")
            .exact_height(60.0)
            .show(ctx, |ui| playback_bar::show(ui, &mut self.state, &self.cmd_tx));

        egui::SidePanel::left("track_list")
            .resizable(true)
            .default_width(260.0)
            .show(ctx, |ui| track_list::show(ui, &mut self.state, &self.cmd_tx));

        egui::SidePanel::right("esp_config")
            .resizable(true)
            .default_width(220.0)
            .show(ctx, |ui| esp_config::show(ui, &mut self.state, &self.cmd_tx));

        egui::CentralPanel::default()
            .show(ctx, |ui| esp_grid::show(ui, &self.state));

        // Handle drag and drop files
        if !ctx.input(|i| i.raw.dropped_files.is_empty()) {
            let files = ctx.input(|i| i.raw.dropped_files.clone());
            let mut paths = vec![];
            
            for file in files {
                if let Some(path) = file.path {
                    if path.is_dir() {
                        // If they drop a folder, change music directory directly
                        let _ = self.cmd_tx.try_send(Cmd::ChangeMusicFolder(path.to_string_lossy().to_string()));
                    } else {
                        // Gather single track files
                        paths.push(path);
                    }
                }
            }
            
            if !paths.is_empty() {
                let _ = self.cmd_tx.try_send(Cmd::AddTracks(paths));
            }
        }

        // Poll for new events 10x per second
        ctx.request_repaint_after(Duration::from_millis(100));
    }
}
