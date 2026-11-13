#include "appstyle.h"

QString appStyleSheet()
{
    return QStringLiteral(R"(
QDialog { background: #0e1218; color: #e8ecf2; }
QWidget#page { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #0e1218, stop:1 #161b24); }
QFrame#card, QFrame#restBar { background: rgba(22,26,34,0.95); border: 1px solid rgba(255,255,255,0.08); border-radius: 22px; }
QLabel { color: #e8ecf2; }
QLabel#title { color: #f4f6fa; font-size: 17pt; font-weight: 800; }
QLabel#subtitle, QLabel#muted, QLabel#restLabel { color: #8a93a0; }
QLineEdit, QComboBox, QSpinBox, QDateEdit, QTimeEdit { background: #141820; color: #e8ecf2; border: 1px solid #262d39; border-radius: 12px; padding: 7px 12px; }
QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDateEdit:focus, QTimeEdit:focus { border: 1px solid #f5a524; }
QComboBox QAbstractItemView { background: #141820; color: #e8ecf2; selection-background-color: #f5a524; selection-color: #101418; }
QCheckBox { color: #8a93a0; }
QGroupBox { color: #e8ecf2; border: 1px solid #262d39; border-radius: 16px; margin-top: 14px; padding-top: 10px; }
QGroupBox::title { subcontrol-origin: margin; left: 12px; color: #8a93a0; }
QPushButton { border: none; border-radius: 14px; padding: 8px 14px; font-weight: 600; background: #222832; color: #e8ecf2; }
QPushButton:hover { background: #2b3340; }
QPushButton[variant="ghost"] { background: #222832; color: #e8ecf2; }
QPushButton[variant="ghost"]:hover { background: #2b3340; }
QPushButton[variant="teal"] { background: #22c3b8; color: #101418; }
QPushButton[variant="teal"]:hover { background: #3dd6cb; }
QPushButton[variant="red"] { background: rgba(224,69,123,0.20); color: #ff7aa6; }
QPushButton[variant="tab"] { background: #222832; color: #8a93a0; border-radius: 18px; padding: 8px 18px; }
QPushButton[variant="tab"]:checked { background: #f5a524; color: #101418; }
QPushButton[variant="chip"] { background: #222832; color: #8a93a0; border-radius: 16px; padding: 6px 12px; min-width: 40px; }
QPushButton[variant="chip"]:checked { background: #f5a524; color: #101418; }
QPushButton[stat="true"] { border-radius: 18px; padding: 12px; background: #222832; color: #e8ecf2; }
QTableWidget { background: #141820; color: #e8ecf2; border: none; gridline-color: #232a35; alternate-background-color: #171c26; }
QHeaderView::section { background: #141820; color: #8a93a0; border: none; padding: 6px; font-weight: 600; }
QScrollArea { background: transparent; border: none; }
QScrollBar:vertical { background: transparent; width: 10px; }
QScrollBar::handle:vertical { background: #2b3340; border-radius: 5px; min-height: 30px; }
)");
}
