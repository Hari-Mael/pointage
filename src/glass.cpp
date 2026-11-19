#include "glass.h"

#include <QEasingCurve>
#include <QEnterEvent>
#include <QGraphicsOpacityEffect>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QRadialGradient>
#include <QTimer>
#include <QVariantAnimation>
#include <cmath>

namespace {
constexpr double kTau = 6.283185307179586;
}

GlassBackdrop::GlassBackdrop(QWidget *parent) : QWidget(parent)
{
    auto *drift = new QVariantAnimation(this);
    drift->setStartValue(0.0);
    drift->setEndValue(1.0);
    drift->setDuration(14000);
    drift->setLoopCount(-1);
    connect(drift, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_phase = value.toDouble();
        update();
    });
    drift->start();
}

void GlassBackdrop::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    const QRectF area(rect());
    const double w = area.width();
    const double h = area.height();

    QLinearGradient base(area.topLeft(), area.bottomRight());
    base.setColorAt(0.0, QColor(10, 26, 21));
    base.setColorAt(1.0, QColor(20, 48, 40));
    painter.fillRect(area, base);

    const double sway = std::sin(m_phase * kTau);
    QRadialGradient amber(QPointF(w * (0.82 + 0.04 * sway), h * 0.18), w * 0.42);
    amber.setColorAt(0.0, QColor(214, 158, 74, 150));
    amber.setColorAt(1.0, QColor(214, 158, 74, 0));
    painter.fillRect(area, amber);

    const double sway2 = std::cos(m_phase * kTau);
    QRadialGradient teal(QPointF(w * (0.18 + 0.05 * sway2), h * 0.92), w * 0.5);
    teal.setColorAt(0.0, QColor(45, 170, 135, 120));
    teal.setColorAt(1.0, QColor(45, 170, 135, 0));
    painter.fillRect(area, teal);
}

GlassCard::GlassCard(QWidget *parent) : QFrame(parent)
{
    setObjectName(QStringLiteral("card"));
}

void GlassCard::setGlow(qreal value)
{
    m_glow = value;
    update();
}

void GlassCard::enterEvent(QEnterEvent *event)
{
    animateGlow(1.0);
    QFrame::enterEvent(event);
}

void GlassCard::leaveEvent(QEvent *event)
{
    animateGlow(0.0);
    QFrame::leaveEvent(event);
}

void GlassCard::animateGlow(qreal target)
{
    if (m_glowAnim) {
        m_glowAnim->stop();
        delete m_glowAnim;
    }
    m_glowAnim = new QPropertyAnimation(this, "glow", this);
    m_glowAnim->setDuration(260);
    m_glowAnim->setStartValue(m_glow);
    m_glowAnim->setEndValue(target);
    m_glowAnim->setEasingCurve(QEasingCurve::OutCubic);
    m_glowAnim->start();
}

void GlassCard::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF area = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    QPainterPath path;
    path.addRoundedRect(area, 18.0, 18.0);

    QLinearGradient fill(area.topLeft(), area.bottomRight());
    fill.setColorAt(0.0, QColor(255, 255, 255, 34));
    fill.setColorAt(1.0, QColor(255, 255, 255, 14));
    painter.fillPath(path, fill);

    QLinearGradient sheen(area.topLeft(), QPointF(area.left(), area.top() + 48.0));
    sheen.setColorAt(0.0, QColor(255, 255, 255, 30));
    sheen.setColorAt(1.0, QColor(255, 255, 255, 0));
    painter.fillPath(path, sheen);

    const QColor border = m_glow > 0.01
        ? QColor(52, 211, 153, static_cast<int>(40 + m_glow * 120))
        : QColor(255, 255, 255, 36);
    QPen pen(border);
    pen.setWidthF(1.2);
    painter.setPen(pen);
    painter.drawPath(path);
}

namespace GlassAnim {

void fadeIn(QWidget *widget, int delayMs, int durationMs)
{
    auto *effect = new QGraphicsOpacityEffect(widget);
    effect->setOpacity(0.0);
    widget->setGraphicsEffect(effect);

    auto *anim = new QPropertyAnimation(effect, "opacity", effect);
    anim->setDuration(durationMs);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setEasingCurve(QEasingCurve::OutCubic);

    QObject::connect(anim, &QPropertyAnimation::finished, widget, [widget]() {
        QTimer::singleShot(0, widget, [widget]() { widget->setGraphicsEffect(nullptr); });
    });
    QTimer::singleShot(delayMs, anim, [anim]() { anim->start(); });
}

void staggerIn(const QList<QWidget *> &widgets, int stepMs)
{
    int delay = 0;
    for (QWidget *widget : widgets) {
        fadeIn(widget, delay);
        delay += stepMs;
    }
}

void countUp(QPushButton *button, int target, std::function<QString(int)> format)
{
    const QString name = QStringLiteral("countUp");
    if (auto *running = button->findChild<QVariantAnimation *>(name, Qt::FindDirectChildrenOnly)) {
        running->stop();
        delete running;
    }

    const int from = button->property("shownValue").toInt();
    if (from == target) {
        button->setText(format(target));
        return;
    }

    auto *anim = new QVariantAnimation(button);
    anim->setObjectName(name);
    anim->setStartValue(from);
    anim->setEndValue(target);
    anim->setDuration(600);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    QObject::connect(anim, &QVariantAnimation::valueChanged, button, [button, format](const QVariant &value) {
        const int shown = value.toInt();
        button->setProperty("shownValue", shown);
        button->setText(format(shown));
    });
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

}
