use serde::{Deserialize, Serialize};
use std::path::PathBuf;

// A single ESP's id + state value
// state is f32 so 0.5 = dim works without any code changes later
#[derive(Clone, Serialize, Deserialize)]
pub struct EspSignal {
    pub id:    u32,
    pub state: f32,   // 0.0 = off, 1.0 = on
}

// A track entry from the music folder
#[derive(Clone, Serialize)]
pub struct Track {
    pub name: String,
    pub path: PathBuf,
}

// Commands the GUI sends to the worker thread
pub enum Cmd {
    Play(usize),            // index into tracks vec
    Pause,
    Stop,
    CriticalStop { total: u32, required: u32 },
    SetTotalEsps(u32),
    SetRequiredEsps(u32),
    RefreshTracks,
    SaveScene(String),      // future: save current signals as named preset
    FireScene(String),      // future: replay a named preset
    SetEspState(u32, f32),  // future: per-ESP manual override
    ChangeMusicFolder(String),
    AddTracks(Vec<PathBuf>),
}

// Events the worker thread sends back to the GUI
pub enum AppEvent {
    TracksLoaded(Vec<Track>),
    PlaybackStarted(String),
    PlaybackStopped,
    PlaybackTick { position_ms: u64, duration_ms: u64 }, // future: music sync
    EspFired(Vec<EspSignal>),
    Error(String),
}

// Snapshot of what the GUI renders
// (GUI never touches AppState directly — only via events)
pub struct UiState {
    pub tracks:         Vec<Track>,
    pub current_track:  Option<usize>,
    pub playing:        bool,
    pub total_esps:     u32,
    pub required_esps:  u32,
    pub last_signals:   Vec<EspSignal>,
    pub status_text:    String,
    pub error:          Option<String>,
}

impl Default for UiState {
    fn default() -> Self {
        Self {
            tracks: Vec::new(),
            current_track: None,
            playing: false,
            total_esps: 10,
            required_esps: 3,
            last_signals: Vec::new(),
            status_text: "Initialized".into(),
            error: None,
        }
    }
}
