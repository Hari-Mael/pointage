#include "appstyle.h"

QString appStyleSheet()
{
    return QStringLiteral(R"(
QDialog, QMessageBox { background: #0a1613; color: #eaf6f1; }
QFrame#card { background: transparent; border: none; }
QFrame#restBar {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 rgba(52,211,153,0.30), stop:1 rgba(245,176,65,0.16));
    border: 1px solid rgba(255,255,255,0.14);
    border-radius: 18px;
}
QLabel { color: #eaf6f1; background: transparent; }
QLabel#title, QLabel#welcome { color: #f4fbf8; font-size: 17pt; font-weight: 800; }
QLabel#subtitle, QLabel#muted { color: #9fbfb4; }
QLabel#restLabel { color: #f4fbf8; font-weight: 600; }
QLineEdit, QComboBox, QSpinBox, QDateEdit, QTimeEdit {
    background: rgba(0,0,0,0.28); color: #eaf6f1;
    border: 1px solid rgba(255,255,255,0.12); border-radius: 12px; padding: 7px 12px;
}
QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDateEdit:focus, QTimeEdit:focus { border: 1px solid #34d399; }
QComboBox QAbstractItemView {
    background: #0f211c; color: #eaf6f1;
    selection-background-color: #34d399; selection-color: #04140e;
    border: 1px solid rgba(255,255,255,0.12);
}
QCheckBox { color: #9fbfb4; }
QGroupBox { color: #eaf6f1; border: 1px solid rgba(255,255,255,0.12); border-radius: 16px; margin-top: 14px; padding-top: 10px; }
QGroupBox::title { subcontrol-origin: margin; left: 12px; color: #9fbfb4; }
QPushButton { border: none; border-radius: 14px; padding: 8px 14px; font-weight: 600; background: rgba(255,255,255,0.10); color: #eaf6f1; }
QPushButton:hover { background: rgba(255,255,255,0.18); }
QPushButton:pressed { background: rgba(255,255,255,0.06); }
QPushButton[variant="teal"] { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #34d399, stop:1 #2dd4bf); color: #04140e; }
QPushButton[variant="teal"]:hover { background: #5ee0ad; }
QPushButton[variant="red"] { background: rgba(244,63,94,0.22); color: #ff8fa3; }
QPushButton[variant="ghost"] { background: rgba(255,255,255,0.08); color: #eaf6f1; border: 1px solid rgba(255,255,255,0.14); }
QPushButton[variant="ghost"]:hover { background: rgba(255,255,255,0.16); }
QPushButton[variant="tab"] { background: rgba(255,255,255,0.08); color: #9fbfb4; border-radius: 18px; padding: 8px 18px; }
QPushButton[variant="tab"]:checked { background: #34d399; color: #04140e; }
QPushButton[variant="chip"] { background: rgba(255,255,255,0.08); color: #9fbfb4; border-radius: 16px; padding: 6px 12px; min-width: 40px; }
QPushButton[variant="chip"]:checked { background: #34d399; color: #04140e; }
QPushButton[stat="true"] { border-radius: 18px; padding: 14px; font-size: 11pt; background: rgba(255,255,255,0.08); color: #eaf6f1; border: 1px solid rgba(255,255,255,0.12); }
QPushButton[stat="true"]:hover { background: rgba(255,255,255,0.14); border: 1px solid rgba(52,211,153,0.6); }
QTableWidget { background: rgba(4,14,11,0.35); color: #eaf6f1; border: none; gridline-color: rgba(255,255,255,0.06); alternate-background-color: rgba(255,255,255,0.03); }
QHeaderView::section { background: rgba(255,255,255,0.06); color: #9fbfb4; border: none; padding: 6px; font-weight: 600; }
QScrollArea { background: transparent; border: none; }
QScrollBar:vertical { background: transparent; width: 10px; }
QScrollBar::handle:vertical { background: rgba(255,255,255,0.18); border-radius: 5px; min-height: 30px; }
QToolTip { background: #0f211c; color: #eaf6f1; border: 1px solid rgba(52,211,153,0.4); padding: 4px; }
QDialog#loginDialog { background: #0a1613; }
QFrame#loginCard { background: rgba(255,255,255,0.07); border: 1px solid rgba(255,255,255,0.14); border-radius: 28px; }
QLabel#iconCircle { background: rgba(255,255,255,0.10); border: 1px solid rgba(255,255,255,0.18); border-radius: 36px; font-size: 24pt; }
QLineEdit#embossed { background: rgba(0,0,0,0.28); border: 1px solid rgba(255,255,255,0.12); border-radius: 16px; padding: 12px 16px; color: #eaf6f1; }
QLineEdit#embossed:focus { border: 1px solid #34d399; }
QPushButton#embossButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #34d399, stop:1 #2dd4bf); color: #04140e; border-radius: 16px; padding: 12px 16px; font-weight: 800; letter-spacing: 1px; }
QPushButton#linkButton { background: transparent; color: #34d399; border: 0; padding: 4px 6px; font-weight: 600; }
)");
}
