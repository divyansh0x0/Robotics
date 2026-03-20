use rodio::{Decoder, OutputStream, OutputStreamHandle, Sink};
use std::fs::File;
use std::io::BufReader;
use std::path::Path;
use std::time::{Duration, Instant};

pub struct Player {
    _stream: OutputStream,          // must be kept alive
    handle:  OutputStreamHandle,
    sink:    Option<Sink>,
    last_played: Option<Instant>,
    accumulated_pos: Duration,
}

impl Player {
    pub fn new() -> Result<Self, String> {
        let (_stream, handle) = OutputStream::try_default().map_err(|e| e.to_string())?;
        Ok(Self {
            _stream,
            handle,
            sink: None,
            last_played: None,
            accumulated_pos: Duration::ZERO,
        })
    }

    pub fn play(&mut self, path: &Path) -> Result<u64, String> {
        self.stop();
        let file   = BufReader::new(File::open(path).map_err(|e| e.to_string())?);
        let source = Decoder::new(file).map_err(|e| e.to_string())?;
        
        use rodio::Source;
        let duration_ms = source.total_duration().map(|d| d.as_millis() as u64).unwrap_or(0);

        let sink   = Sink::try_new(&self.handle).map_err(|e| e.to_string())?;
        sink.append(source);
        sink.play();
        self.sink = Some(sink);
        
        self.accumulated_pos = Duration::ZERO;
        self.last_played = Some(Instant::now());
        
        Ok(duration_ms)
    }

    pub fn pause(&mut self) {
        if let Some(s) = &self.sink {
            s.pause();
            if let Some(played) = self.last_played.take() {
                self.accumulated_pos += played.elapsed();
            }
        }
    }

    pub fn resume(&mut self) {
        if let Some(s) = &self.sink {
            s.play();
            if self.last_played.is_none() {
                self.last_played = Some(Instant::now());
            }
        }
    }

    pub fn stop(&mut self) {
        self.sink.take();
        self.last_played = None;
        self.accumulated_pos = Duration::ZERO;
    }

    pub fn is_paused(&self) -> bool { self.sink.as_ref().map(|s| s.is_paused()).unwrap_or(true) }

    pub fn position(&self) -> Option<u64> {
        if self.sink.is_none() {
            return None;
        }
        let pos = self.accumulated_pos + self.last_played.map(|p| p.elapsed()).unwrap_or(Duration::ZERO);
        Some(pos.as_millis() as u64)
    }

    pub fn seek(&mut self, pos_ms: u64) {
        if let Some(s) = &self.sink {
            let dur = Duration::from_millis(pos_ms);
            let _ = s.try_seek(dur);
            self.accumulated_pos = dur;
            if self.last_played.is_some() {
                self.last_played = Some(Instant::now());
            }
        }
    }
}
