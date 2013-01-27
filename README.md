TrayPost
========

TrayPost is simple desktop notifier application accessible from tray.

The application saves lines passed to standard input to temporary log and shows
number of new lines in tray icon. Clicking on the tray icon resets the message
counter or shows log.

Activating an item in log (double-click, press enter key) prints it on standard
output.

Command Line
------------

    Usage: /home/lukas/dev/traypost/build/traypost [Options]
    Options:
      -h, --help                    Print help.

      -i, --icon {file name}        Tray icon
      -t, --text {icon text}        Tray icon text
      -c, --color {color=black}     Tray icon text color
      -o, --outline {color=white}   Tray icon text outline color
      -f, --font {font}             Tray icon text font (e.g. 'DejaVu Sans, 10, bold, underline')
                                    Font options: italic, bold, overline, underline, strikeout
      -T, --tooltip {tooltip text}  Tray icon default tool tip text

      --timeout {milliseconds}      Message show timeout.
      --time-format {format}        Time format for messages (e.g. 'dd.MM.yyyy hh:mm:ss.zzz')
      --message-format {format}     Format for messages (HTML; %1 is message, %2 is message time)
                                    Example: '<p><small><b>%1</b></small><br />%2</p>'

      --record-end                  Record end of stdin.
      --show-log                    Show log dialog at start.
      --select                      Open log dialog and exit after it is closed.

