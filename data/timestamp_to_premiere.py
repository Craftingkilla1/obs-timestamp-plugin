#!/usr/bin/env python3
"""
OBS Timestamp to Premiere Pro XML Converter

Converts OBS timestamp markers (JSON Lines format) to Premiere Pro marker XML format.
"""

import os
import sys
import json
import argparse
import xml.etree.ElementTree as ET
from xml.dom import minidom
from datetime import datetime

# Premiere Pro color codes (32-bit ARGB values)
COLOR_MAP = {
    "blue": "4294741314",
    "cyan": "4294940928",
    "green": "4278255360",
    "yellow": "4278255615",
    "red": "4294901760",
    "magenta": "4294902015",
    "purple": "4286578816",
    "orange": "4294924800",
}

def parse_arguments():
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        description='Convert OBS timestamps to Premiere Pro markers',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s timestamps.jsonl                          # Auto-detect video and output filename
  %(prog)s timestamps.jsonl markers.xml              # Manual output filename
  %(prog)s timestamps.jsonl --fps 60                 # Override FPS (auto-detected from metadata)
  %(prog)s timestamps.jsonl --sequence-name "My Recording"
        """
    )
    parser.add_argument('input', help='Input timestamp file (JSON Lines format)')
    parser.add_argument('output', nargs='?', default=None,
                        help='Output XML file for Premiere Pro (optional, auto-detected from metadata)')
    parser.add_argument('--fps', type=float, default=60,
                        help='Frames per second (default: 60, or auto-detected from metadata)')
    parser.add_argument('--sequence-name', default=None,
                        help='Name for the sequence (default: based on input filename)')
    parser.add_argument('--width', type=int, default=1920,
                        help='Video width (default: 1920)')
    parser.add_argument('--height', type=int, default=1080,
                        help='Video height (default: 1080)')
    return parser.parse_args()

def parse_timestamps(file_path):
    """
    Parse timestamps from JSON Lines file.

    Returns:
        Tuple of (metadata_dict, timestamps_list)
        metadata_dict contains recording_path, timestamp, fps info
        timestamps_list contains dicts with keys: timestamp_ms, comment, name, color
    """
    timestamps = []
    metadata = {}

    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            for line_num, line in enumerate(f, 1):
                line = line.strip()
                if not line:
                    continue

                try:
                    data = json.loads(line)

                    # Check if this is the metadata line (first line)
                    if 'metadata' in data:
                        metadata = data['metadata']
                        print(f"Found metadata: recording_path={metadata.get('recording_path', 'N/A')}")
                        continue

                    # Validate required fields for timestamp entries
                    if 'timestamp_ms' not in data:
                        print(f"Warning: Line {line_num} missing 'timestamp_ms' field, skipping")
                        continue

                    # Extract fields with defaults
                    timestamp = {
                        'timestamp_ms': int(data['timestamp_ms']),
                        'comment': data.get('comment', ''),
                        'name': data.get('name', ''),
                        'color': data.get('color', 'blue')
                    }

                    timestamps.append(timestamp)

                except json.JSONDecodeError as e:
                    print(f"Warning: Line {line_num} is not valid JSON: {e}")
                    continue
                except (ValueError, TypeError) as e:
                    print(f"Warning: Line {line_num} has invalid data: {e}")
                    continue

    except FileNotFoundError:
        print(f"Error: Input file '{file_path}' not found")
        return None, None
    except Exception as e:
        print(f"Error reading file: {e}")
        return None, None

    return metadata, timestamps

def ms_to_frames(milliseconds, fps):
    """Convert milliseconds to frame number."""
    return int((milliseconds / 1000.0) * fps)

def get_color_code(color_name):
    """Get Premiere Pro color code from color name."""
    return COLOR_MAP.get(color_name.lower(), COLOR_MAP["blue"])

def find_latest_video_file(recording_dir, metadata_timestamp):
    """
    Find the most recent video file in the recording directory.

    Args:
        recording_dir: Path to the recording directory
        metadata_timestamp: Timestamp string from metadata (format: "YYYY-MM-DD HH:MM:SS")

    Returns:
        Path to the most recent video file, or None if not found
    """
    if not recording_dir or not os.path.exists(recording_dir):
        return None

    # Common video extensions
    video_extensions = ('.mp4', '.mkv', '.flv', '.mov', '.avi', '.ts')

    # Parse the metadata timestamp
    try:
        from datetime import datetime
        metadata_time = datetime.strptime(metadata_timestamp, "%Y-%m-%d %H:%M:%S")
    except:
        metadata_time = None

    # Find all video files
    video_files = []
    try:
        for filename in os.listdir(recording_dir):
            if filename.lower().endswith(video_extensions):
                filepath = os.path.join(recording_dir, filename)
                # Get file modification time
                mtime = os.path.getmtime(filepath)
                video_files.append((filepath, mtime))
    except Exception as e:
        print(f"Error scanning directory: {e}")
        return None

    if not video_files:
        return None

    # Sort by modification time (newest first)
    video_files.sort(key=lambda x: x[1], reverse=True)

    # If we have a metadata timestamp, find the file closest to that time
    if metadata_time:
        metadata_epoch = metadata_time.timestamp()
        # Find file with closest modification time (within 5 minutes)
        for filepath, mtime in video_files:
            time_diff = abs(mtime - metadata_epoch)
            if time_diff < 300:  # Within 5 minutes
                return filepath

    # Otherwise return the most recent file
    return video_files[0][0]

def create_premiere_xml(timestamps, output_path, fps=60, sequence_name=None, width=1920, height=1080):
    """
    Create Premiere Pro compatible XML with markers.

    Args:
        timestamps: List of timestamp dicts
        output_path: Path to save XML file
        fps: Frames per second
        sequence_name: Name for the sequence
        width: Video width
        height: Video height
    """
    if not timestamps:
        print("Error: No timestamps to convert")
        return False

    # Determine if NTSC framerate
    ntsc = fps in [23.976, 29.97, 59.94]
    timebase = int(fps) if not ntsc else int(fps * 1.001)

    # Default sequence name
    if not sequence_name:
        sequence_name = f"OBS Markers ({datetime.now().strftime('%Y-%m-%d %H:%M')})"

    # Calculate duration (last marker + 60 seconds buffer)
    max_timestamp_ms = max(t['timestamp_ms'] for t in timestamps)
    duration = ms_to_frames(max_timestamp_ms + 60000, fps)

    # Create root element with DOCTYPE
    root = ET.Element('xmeml', version="4")

    # Create sequence
    sequence = ET.SubElement(root, 'sequence', {
        'id': 'sequence',
        'explodedTracks': 'true'
    })

    # Add UUID (can be generated or static)
    ET.SubElement(sequence, 'uuid').text = 'obs-timestamp-markers-sequence'

    # Duration
    ET.SubElement(sequence, 'duration').text = str(duration)

    # Rate
    rate = ET.SubElement(sequence, 'rate')
    ET.SubElement(rate, 'timebase').text = str(timebase)
    ET.SubElement(rate, 'ntsc').text = 'TRUE' if ntsc else 'FALSE'

    # Name
    ET.SubElement(sequence, 'name').text = sequence_name

    # Media section
    media = ET.SubElement(sequence, 'media')

    # Video track
    video = ET.SubElement(media, 'video')
    video_format = ET.SubElement(video, 'format')
    sample_chars = ET.SubElement(video_format, 'samplecharacteristics')

    # Video rate
    video_rate = ET.SubElement(sample_chars, 'rate')
    ET.SubElement(video_rate, 'timebase').text = str(timebase)
    ET.SubElement(video_rate, 'ntsc').text = 'TRUE' if ntsc else 'FALSE'

    # Video codec
    codec = ET.SubElement(sample_chars, 'codec')
    ET.SubElement(codec, 'name').text = 'Apple ProRes 422'

    # Video dimensions
    ET.SubElement(sample_chars, 'width').text = str(width)
    ET.SubElement(sample_chars, 'height').text = str(height)
    ET.SubElement(sample_chars, 'anamorphic').text = 'FALSE'
    ET.SubElement(sample_chars, 'pixelaspectratio').text = 'square'
    ET.SubElement(sample_chars, 'fielddominance').text = 'none'
    ET.SubElement(sample_chars, 'colordepth').text = '24'

    # Video track
    video_track = ET.SubElement(video, 'track')
    ET.SubElement(video_track, 'enabled').text = 'TRUE'
    ET.SubElement(video_track, 'locked').text = 'FALSE'

    # Generator item (invisible color matte that holds the markers)
    gen_item = ET.SubElement(video_track, 'generatoritem', {'id': 'clipitem-1'})
    ET.SubElement(gen_item, 'name').text = 'OBS Marker Holder'
    ET.SubElement(gen_item, 'enabled').text = 'TRUE'
    ET.SubElement(gen_item, 'duration').text = str(duration)

    gen_rate = ET.SubElement(gen_item, 'rate')
    ET.SubElement(gen_rate, 'timebase').text = str(timebase)
    ET.SubElement(gen_rate, 'ntsc').text = 'TRUE' if ntsc else 'FALSE'

    ET.SubElement(gen_item, 'start').text = '0'
    ET.SubElement(gen_item, 'end').text = str(duration)
    ET.SubElement(gen_item, 'in').text = '0'
    ET.SubElement(gen_item, 'out').text = str(duration)
    ET.SubElement(gen_item, 'alphatype').text = 'none'

    # Add color matte effect
    effect = ET.SubElement(gen_item, 'effect')
    ET.SubElement(effect, 'name').text = 'Color'
    ET.SubElement(effect, 'effectid').text = 'Color'
    ET.SubElement(effect, 'effectcategory').text = 'Matte'
    ET.SubElement(effect, 'effecttype').text = 'generator'
    ET.SubElement(effect, 'mediatype').text = 'video'

    parameter = ET.SubElement(effect, 'parameter', {'authoringApp': 'PremierePro'})
    ET.SubElement(parameter, 'parameterid').text = 'fillcolor'
    ET.SubElement(parameter, 'name').text = 'Color'
    value = ET.SubElement(parameter, 'value')
    ET.SubElement(value, 'alpha').text = '0'
    ET.SubElement(value, 'red').text = '0'
    ET.SubElement(value, 'green').text = '0'
    ET.SubElement(value, 'blue').text = '0'

    # Add opacity filter to make it invisible
    filter_elem = ET.SubElement(gen_item, 'filter')
    opacity_effect = ET.SubElement(filter_elem, 'effect')
    ET.SubElement(opacity_effect, 'name').text = 'Opacity'
    ET.SubElement(opacity_effect, 'effectid').text = 'opacity'
    ET.SubElement(opacity_effect, 'effectcategory').text = 'motion'
    ET.SubElement(opacity_effect, 'effecttype').text = 'motion'
    ET.SubElement(opacity_effect, 'mediatype').text = 'video'

    opacity_param = ET.SubElement(opacity_effect, 'parameter', {'authoringApp': 'PremierePro'})
    ET.SubElement(opacity_param, 'parameterid').text = 'opacity'
    ET.SubElement(opacity_param, 'name').text = 'opacity'
    ET.SubElement(opacity_param, 'value').text = '0'

    # Add markers to generator item
    for ts in timestamps:
        frame = ms_to_frames(ts['timestamp_ms'], fps)
        color_code = get_color_code(ts['color'])

        marker = ET.SubElement(gen_item, 'marker')
        ET.SubElement(marker, 'comment').text = ts['comment']
        ET.SubElement(marker, 'name').text = ts['name']
        ET.SubElement(marker, 'in').text = str(frame)
        ET.SubElement(marker, 'out').text = '-1'
        ET.SubElement(marker, 'pproColor').text = color_code

    # Audio section
    audio = ET.SubElement(media, 'audio')
    ET.SubElement(audio, 'numOutputChannels').text = '2'

    audio_format = ET.SubElement(audio, 'format')
    audio_sample_chars = ET.SubElement(audio_format, 'samplecharacteristics')
    ET.SubElement(audio_sample_chars, 'depth').text = '16'
    ET.SubElement(audio_sample_chars, 'samplerate').text = '48000'

    # Add basic audio tracks
    for i in range(2):
        audio_track = ET.SubElement(audio, 'track')
        ET.SubElement(audio_track, 'enabled').text = 'TRUE'
        ET.SubElement(audio_track, 'locked').text = 'FALSE'
        ET.SubElement(audio_track, 'outputchannelindex').text = str(i + 1)

    # Timecode
    timecode = ET.SubElement(sequence, 'timecode')
    timecode_rate = ET.SubElement(timecode, 'rate')
    ET.SubElement(timecode_rate, 'timebase').text = str(timebase)
    ET.SubElement(timecode_rate, 'ntsc').text = 'TRUE' if ntsc else 'FALSE'
    ET.SubElement(timecode, 'string').text = '00:00:00:00'
    ET.SubElement(timecode, 'frame').text = '0'
    ET.SubElement(timecode, 'displayformat').text = 'NDF'

    # Add markers at sequence level too (for better compatibility)
    for ts in timestamps:
        frame = ms_to_frames(ts['timestamp_ms'], fps)
        color_code = get_color_code(ts['color'])

        marker = ET.SubElement(sequence, 'marker')
        ET.SubElement(marker, 'comment').text = ts['comment']
        ET.SubElement(marker, 'name').text = ts['name']
        ET.SubElement(marker, 'in').text = str(frame)
        ET.SubElement(marker, 'out').text = '-1'
        ET.SubElement(marker, 'pproColor').text = color_code

    # Convert to pretty XML string
    xml_string = ET.tostring(root, encoding='unicode')
    pretty_xml = minidom.parseString(xml_string).toprettyxml(indent='  ')

    # Add DOCTYPE declaration
    final_xml = '<?xml version="1.0" encoding="UTF-8"?>\n'
    final_xml += '<!DOCTYPE xmeml>\n'
    final_xml += '\n'.join(pretty_xml.split('\n')[1:])  # Remove first line (duplicate XML declaration)

    # Write to file
    try:
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(final_xml)
        return True
    except Exception as e:
        print(f"Error writing XML file: {e}")
        return False

def main():
    """Main entry point."""
    args = parse_arguments()

    print(f"OBS Timestamp to Premiere Pro XML Converter")
    print(f"=" * 50)
    print(f"Input file:  {args.input}")
    print()

    # Parse timestamps
    print("Parsing timestamps...")
    metadata, timestamps = parse_timestamps(args.input)

    if timestamps is None:
        return 1

    if not timestamps:
        print("No valid timestamps found in the input file")
        return 1

    print(f"Found {len(timestamps)} timestamp(s)")
    print()

    # Auto-detect FPS from metadata if available
    fps = args.fps
    if metadata and 'fps_num' in metadata and 'fps_den' in metadata:
        fps_num = metadata['fps_num']
        fps_den = metadata['fps_den']
        calculated_fps = fps_num / fps_den
        print(f"Detected FPS from metadata: {fps_num}/{fps_den} = {calculated_fps:.2f}")
        fps = calculated_fps

    # Auto-detect output filename from video file if metadata available
    output_file = args.output
    if metadata and 'recording_path' in metadata and 'timestamp' in metadata:
        recording_dir = metadata['recording_path']
        recording_time = metadata['timestamp']

        print(f"Recording directory: {recording_dir}")
        print(f"Recording timestamp: {recording_time}")
        print()

        # Try to find the matching video file
        video_file = find_latest_video_file(recording_dir, recording_time)
        if video_file:
            print(f"Found matching video: {os.path.basename(video_file)}")
            # Generate XML filename based on video filename
            video_basename = os.path.splitext(os.path.basename(video_file))[0]
            auto_output = os.path.join(recording_dir, f"{video_basename}_markers.xml")
            print(f"Auto-generated output: {auto_output}")
            output_file = auto_output
        else:
            print("Warning: Could not find matching video file")
            if not output_file:
                # Fallback: use input filename with .xml extension in same directory
                output_file = os.path.splitext(args.input)[0] + '_markers.xml'
            print(f"Using output: {output_file}")
    else:
        print(f"No metadata found, using manual settings")
        if not output_file:
            # Fallback: use input filename with .xml extension
            output_file = os.path.splitext(args.input)[0] + '_markers.xml'
        print(f"Output file: {output_file}")

    print(f"FPS:         {fps}")
    print()

    # Display timestamps
    print("Timestamps:")
    for i, ts in enumerate(timestamps, 1):
        seconds = ts['timestamp_ms'] / 1000.0
        print(f"  {i}. {seconds:8.2f}s - {ts['comment']:<30} "
              f"[{ts['name'] or 'no name':<15}] ({ts['color']})")
    print()

    # Generate XML
    print("Generating Premiere Pro XML...")
    success = create_premiere_xml(
        timestamps,
        output_file,
        fps=fps,
        sequence_name=args.sequence_name,
        width=args.width,
        height=args.height
    )

    if success:
        print(f"✓ XML file saved to '{output_file}'")
        print()
        print("To import into Premiere Pro:")
        print("  1. Open Premiere Pro")
        print("  2. Go to File → Import")
        print("  3. Select the XML file")
        print("  4. The markers will be imported into a new sequence")
        return 0
    else:
        print("✗ Failed to generate XML file")
        return 1

if __name__ == "__main__":
    sys.exit(main())
