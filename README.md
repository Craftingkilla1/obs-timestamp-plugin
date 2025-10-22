# OBS Timestamp Plugin

An OBS Studio plugin that captures timestamp markers during recording sessions for easy import into video editors like Adobe Premiere Pro.

## Features

- Press a hotkey during OBS recording to create timestamp markers
- Automatic markers at recording start and end
- Outputs timestamps in JSON Lines format
- Compatible with the included Python converter for Premiere Pro markers

## Installation

### From Release (Recommended)

1. Download the latest release for your platform
2. Extract the files
3. Copy to your OBS plugins directory:
   - **Windows**: `%APPDATA%\obs-studio\plugins\obs-timestamp-plugin\`
   - **Linux**: `~/.config/obs-studio/plugins/obs-timestamp-plugin/`
   - **macOS**: `~/Library/Application Support/obs-studio/plugins/obs-timestamp-plugin/`

### Building from Source

See the parent project's BUILD.md for detailed instructions.

## Usage

1. Open OBS Studio
2. Go to Settings → Hotkeys
3. Find "Create Timestamp Marker" and assign a key
4. Start recording
5. Press your hotkey to create markers
6. Stop recording
7. Timestamps are saved to: `[config]/plugin_config/obs-timestamp-plugin/timestamps.jsonl`

## Output Format

The plugin outputs JSON Lines format:

```json
{"timestamp_ms": 0, "comment": "Recording Start", "name": "", "color": "blue"}
{"timestamp_ms": 15000, "comment": "Marker 1", "name": "", "color": "blue"}
```

## Converting to Premiere Pro Markers

Use the included Python converter:

```bash
python3 timestamp_to_premiere.py timestamps.jsonl markers.xml --fps 60
```

Then import `markers.xml` into Premiere Pro via File → Import.

## License

This plugin is provided as-is for personal and educational use.

## Links

- Full project documentation: [Parent Repository]
- Report issues: [GitHub Issues]
