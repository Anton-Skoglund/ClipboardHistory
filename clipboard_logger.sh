#!/bin/bash
MAX_ENTRIES=5
INTERVAL=3   # check every 3s

HISTORY_DIR="$HOME/.clipboard_history"
TEXT_DIR="$HISTORY_DIR/texts"
IMG_DIR="$HISTORY_DIR/images"
LOG_FILE="$HISTORY_DIR/history.txt"
LAST_TEXT=""
LAST_IMG_HASH=""

mkdir -p "$TEXT_DIR" "$IMG_DIR"
touch "$LOG_FILE"

trim_history() {
    if [ "$(wc -l < "$LOG_FILE")" -gt "$MAX_ENTRIES" ]; then
        tail -n "$MAX_ENTRIES" "$LOG_FILE" > "$LOG_FILE.tmp" && mv "$LOG_FILE.tmp" "$LOG_FILE"
    fi
}

save_text() {
    local content="$1"
    if [ "$content" != "$LAST_TEXT" ]; then
        ts=$(date +%s)
        file="$TEXT_DIR/$ts.txt"
        printf "%s" "$content" > "$file"
        ln -sf "$file" "$HISTORY_DIR/last_text"
        echo "TEXT $file" >> "$LOG_FILE"
        echo "Copied text: ${content:0:50}..."
        LAST_TEXT="$content"
        trim_history
    fi
}

save_image() {
    local file="$IMG_DIR/$(date +%s).png"
    cat > "$file"
    if [ -s "$file" ]; then
        hash=$(sha256sum "$file" | awk '{print $1}')
        if [ "$hash" != "$LAST_IMG_HASH" ]; then
            ln -sf "$file" "$HISTORY_DIR/last_image"
            echo "IMAGE $file" >> "$LOG_FILE"
            echo "Copied image: $file"
            LAST_IMG_HASH="$hash"
            trim_history
        else
            rm "$file"
        fi
    else
        rm "$file"
    fi
}

if [ -n "$WAYLAND_DISPLAY" ]; then
    echo "Running in Wayland (light polling mode)"
    while true; do
        # Only fetch text once
        txt=$(wl-paste --no-newline 2>/dev/null)
        [ -n "$txt" ] && save_text "$txt"

        # Only check images if clipboard offers image/png
        if wl-paste --list-types 2>/dev/null | grep -q "image/png"; then
            wl-paste --type image/png 2>/dev/null | save_image
        fi

        sleep $INTERVAL
    done
elif [ -n "$DISPLAY" ]; then
    echo "Running in X11 (light polling mode)"
    while true; do
        txt=$(xclip -o -selection clipboard 2>/dev/null)
        [ -n "$txt" ] && save_text "$txt"

        if xclip -selection clipboard -t TARGETS -o 2>/dev/null | grep -q "image/png"; then
            xclip -selection clipboard -t image/png -o 2>/dev/null | save_image
        fi

        sleep $INTERVAL
    done
else
    echo "Error: No Wayland or X11 display detected."
    exit 1
fi
