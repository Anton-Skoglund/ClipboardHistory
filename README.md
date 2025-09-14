# Clipboard Logger

## ⚠️Warning⚠️
- The logger is very slow, haven't tried to set up keybinds do on own risk

A clipboard history logger with text and image storage. Runs as a user service via systemd. Includes a C utility (image_list) to quickly view clipboard history.

---

## Installation

Clone the repository:

git clone https://github.com/USER/clipobar.git
cd clipobar

---

## Setup as a User Service

Create the systemd user service directory:

mkdir -p ~/.config/systemd/user

Create the service file:

cat > ~/.config/systemd/user/clipboard-logger.service <<'EOF'
[Unit]
Description=Clipboard Logger

[Service]
ExecStart=%h/projects/clipobar/clipboard_logger.sh
Restart=always

[Install]
WantedBy=default.target
EOF

⚠️ Replace %h/projects/clipobar/clipboard_logger.sh with the full path to your script if needed.

Reload systemd and enable:

systemctl --user daemon-reload
systemctl --user enable clipboard-logger.service
systemctl --user start clipboard-logger.service

Check status:

systemctl --user status clipboard-logger.service

The service will now start at login.

---

## Compiling image_list

Build the C utility:

gcc image_list.c -o image_list

---

## Running image_list from Anywhere

Option 1: Update PATH

echo 'export PATH="$HOME/projects/clipobar:$PATH"' >> ~/.bashrc
source ~/.bashrc

Now just type:

image_list

Option 2: Symlink to /usr/local/bin

sudo ln -s ~/projects/clipobar/image_list /usr/local/bin/image_list

Now available system-wide:

image_list

Option 3: Alias

echo 'alias clipolist="~/projects/clipobar/image_list"' >> ~/.bashrc
source ~/.bashrc

Run with:

clipolist

---

## GNOME Keyboard Shortcut (Optional)

1. Open Settings → Keyboard → View and Customize Shortcuts
2. Scroll down and click Custom Shortcuts → Add Shortcut
3. Name: Clipboard History
4. Command:

image_list

5. Set your preferred key combination (e.g. Ctrl+Alt+C)

Now pressing that key combo will launch the clipboard history viewer.

---

## Usage

- Text entries saved in texts/
- Image entries saved in images/
- Keep lists: text_keep.list, img_keep.list
- Run image_list (or your alias) to show clipboard history

---

## Stopping the Service

systemctl --user stop clipboard-logger.service

---

## License

MIT
