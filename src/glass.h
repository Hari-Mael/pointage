#pragma once

#include <QFrame>
#include <QList>
#include <QWidget>
#include <functional>

class QEnterEvent;
class QPaintEvent;
class QPropertyAnimation;
class QPushButton;

class GlassBackdrop : public QWidget {
    Q_OBJECT
public:
    explicit GlassBackdrop(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    double m_phase = 0.0;
};

class GlassCard : public QFrame {
    Q_OBJECT
    Q_PROPERTY(qreal glow READ glow WRITE setGlow)

public:
    explicit GlassCard(QWidget *parent = nullptr);

    qreal glow() const { return m_glow; }
    void setGlow(qreal value);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void animateGlow(qreal target);

    qreal m_glow = 0.0;
    QPropertyAnimation *m_glowAnim = nullptr;
};

namespace GlassAnim {
void fadeIn(QWidget *widget, int delayMs, int durationMs = 450);
void staggerIn(const QList<QWidget *> &widgets, int stepMs = 90);
void countUp(QPushButton *button, int target, std::function<QString(int)> format);
}
