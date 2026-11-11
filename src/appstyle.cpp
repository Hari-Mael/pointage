#include "appstyle.h"

QString appStyleSheet()
{
    return QStringLiteral(R"(
QWidget#page { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #faf5ec, stop:1 #e8eef6); }
QFrame#card, QFrame#restBar { background: #ffffff; border: 1px solid #ebe6dd; border-radius: 22px; }
QLabel#title { color: #1d2733; font-size: 17pt; font-weight: 800; }
QLabel#subtitle, QLabel#muted, QLabel#restLabel { color: #7a8593; }
QPushButton[variant="ghost"] { border: none; border-radius: 14px; padding: 8px 14px; font-weight: 600; background: #f3f1ec; color: #1d2733; }
QPushButton[variant="ghost"]:hover { background: #e9e5dc; }
QPushButton[variant="teal"] { border: none; border-radius: 14px; padding: 8px 14px; font-weight: 600; background: #14a39a; color: #ffffff; }
QPushButton[variant="red"] { border: none; border-radius: 14px; padding: 8px 14px; font-weight: 600; background: #fde2e4; color: #d12f3a; }
QPushButton[variant="tab"] { border: none; border-radius: 18px; padding: 8px 18px; font-weight: 600; background: #f3f1ec; color: #1d2733; }
QPushButton[variant="tab"]:checked { background: #1d2733; color: #ffffff; }
QPushButton[variant="chip"] { border: none; border-radius: 16px; padding: 6px 12px; min-width: 40px; font-weight: 600; background: #f3f1ec; color: #1d2733; }
QPushButton[variant="chip"]:checked { background: #f0b040; color: #1d2733; }
QPushButton[stat="true"] { border: none; border-radius: 18px; padding: 12px; font-weight: 600; background: #f3f1ec; color: #1d2733; }
QTableWidget { background: #ffffff; border: none; gridline-color: #f0ede7; }
QHeaderView::section { background: #ffffff; color: #7a8593; border: none; padding: 6px; font-weight: 600; }
)");
}
