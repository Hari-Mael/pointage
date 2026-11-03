#include "theme.h"

#include <QApplication>

void applyTheme(QApplication &app)
{
    app.setStyleSheet(QStringLiteral(R"QSS(
QWidget {
    font-family: "Segoe UI", "Roboto", "Helvetica", sans-serif;
    font-size: 10pt;
    color: #1d2a33;
    background: transparent;
}
QDialog, QMainWindow, QWidget#page {
    background: #d9dee6;
}
QFrame#card {
    background: #eef1f6;
    border: 1px solid #d3d9e2;
    border-radius: 18px;
}
QFrame#restBar {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #0b3b5c, stop:1 #0f6b73);
    border-radius: 18px;
}
QLabel#restLabel { color: #ffffff; font-weight: 600; }
QLabel#title { color: #0b3b5c; font-size: 16pt; font-weight: 800; }
QLabel#subtitle, QLabel#muted { color: #5e7587; font-size: 9pt; }
QLabel#welcome { color: #0b3b5c; font-size: 20pt; font-weight: 800; }
QLabel#icon { font-size: 28pt; }
QLineEdit, QComboBox, QSpinBox, QDateEdit {
    background: #f4f6f9;
    border: 1px solid #cdd4de;
    border-radius: 12px;
    padding: 7px 10px;
    min-height: 22px;
    color: #1d2a33;
}
QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDateEdit:focus { border: 1px solid #148f8a; }
QComboBox::drop-down, QDateEdit::drop-down, QSpinBox::up-button, QSpinBox::down-button { border: 0; width: 20px; }
QComboBox QAbstractItemView {
    background: #ffffff;
    selection-background-color: #0b4159;
    selection-color: #ffffff;
    border: 1px solid #cdd4de;
}
QPushButton {
    background: #0b4159;
    color: #ffffff;
    border: 0;
    border-radius: 12px;
    padding: 9px 16px;
    font-weight: 600;
}
QPushButton:hover { background: #0e5070; }
QPushButton:pressed { background: #082f44; }
QPushButton[variant="teal"] { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #14a39a, stop:1 #10b2a6); }
QPushButton[variant="red"] { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #d63d43, stop:1 #f0676b); }
QPushButton[variant="blue"] { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #1e8bff, stop:1 #29a7ff); }
QPushButton[variant="ghost"] { background: #f4f6f9; color: #1d2a33; border: 1px solid #cdd4de; }
QPushButton[variant="chip"] { background: #2a6a7c; padding: 6px 12px; border-radius: 10px; font-size: 9pt; }
QPushButton[variant="chip"]:checked { background: #ffffff; color: #0b3b5c; }
QPushButton[variant="tab"] { background: #c9d1dc; color: #1d2a33; padding: 8px 16px; }
QPushButton[variant="tab"]:checked { background: #0b4159; color: #ffffff; }
QPushButton[variant="choice"] { background: #f4f6f9; color: #1d2a33; border: 2px solid #cdd4de; }
QPushButton[variant="choice"]:checked { background: #0b4159; color: #ffffff; border-color: #0b4159; }
QPushButton[variant="presentChoice"] { background: #f4f6f9; color: #1b8a4b; border: 2px solid #cdd4de; }
QPushButton[variant="presentChoice"]:checked { background: #14a39a; color: #ffffff; border-color: #14a39a; }
QPushButton[variant="absentChoice"] { background: #f4f6f9; color: #d12f3a; border: 2px solid #cdd4de; }
QPushButton[variant="absentChoice"]:checked { background: #d63d43; color: #ffffff; border-color: #d63d43; }
QPushButton[small="true"] { padding: 4px 8px; font-size: 8pt; border-radius: 8px; }
QPushButton[stat="true"] { padding: 12px 18px; min-width: 140px; }
QTabWidget::pane { border: 0; }
QTableWidget {
    background: #ffffff;
    border: 0;
    border-radius: 12px;
    gridline-color: #e3e6ea;
}
QHeaderView::section {
    background: #0b3b5c;
    color: #ffffff;
    font-weight: 600;
    border: 0;
    padding: 5px 4px;
}
QTableCornerButton::section { background: #0b3b5c; border: 0; }
QScrollBar:horizontal { height: 8px; background: #e6eaf0; border-radius: 4px; }
QScrollBar::handle:horizontal { background: #b6c0cc; border-radius: 4px; min-width: 40px; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
QDialog#loginDialog { background: #e6eaf0; }
QFrame#loginCard {
    background: #e6eaf0;
    border: 1px solid #ffffff;
    border-radius: 28px;
}
QLabel#iconCircle {
    background: #e6eaf0;
    border: 1px solid #ffffff;
    border-radius: 36px;
    font-size: 24pt;
}
QLabel#welcome { color: #1d2a33; font-size: 20pt; font-weight: 800; }
QLabel#subtitle { color: #5e7587; font-size: 10pt; }
QLineEdit#embossed {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #d3d9e2, stop:1 #eef1f6);
    border: 1px solid #ffffff;
    border-radius: 16px;
    padding: 12px 16px;
    color: #1d2a33;
}
QLineEdit#embossed:focus { border: 1px solid #1e8bff; }
QPushButton#embossButton {
    background: #e6eaf0;
    color: #1e6fe0;
    border: 1px solid #ffffff;
    border-radius: 16px;
    padding: 12px 16px;
    font-weight: 800;
    letter-spacing: 1px;
}
QPushButton#embossButton:hover { background: #dfe5ee; }
QPushButton#embossButton:pressed { background: #d3d9e2; }
QPushButton#linkButton {
    background: transparent;
    color: #1e6fe0;
    border: 0;
    padding: 4px 6px;
    font-weight: 600;
}
QPushButton#linkButton:hover { color: #0b4ea8; }
QToolTip { background: #0b3b5c; color: #ffffff; border: 0; padding: 4px; }
)QSS"));
}
