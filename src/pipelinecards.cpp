#include "pipelinecards.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include "photo.h"

namespace PipelineCards {

ColumnParts makeColumn(const QString &title, const QColor &dot, QWidget *parent)
{
    ColumnParts parts;
    parts.frame = new QFrame(parent);
    parts.frame->setObjectName(QStringLiteral("pColumn"));
    parts.frame->setMinimumWidth(240);
    parts.frame->setStyleSheet(QStringLiteral(
        "QFrame#pColumn { background: #f3efe5; border-radius: 24px; }"
        "QLabel#pTitle { color: #1d1d1f; font-weight: 800; font-size: 11pt; background: transparent; }"
        "QLabel#pCount { color: #1d1d1f; background: #ffffff; border-radius: 10px; padding: 2px 8px; font-weight: 600; }"));

    parts.layout = new QVBoxLayout(parts.frame);
    parts.layout->setContentsMargins(12, 12, 12, 12);
    parts.layout->setSpacing(10);

    auto *header = new QHBoxLayout;
    auto *dotLabel = new QLabel(QStringLiteral("●"), parts.frame);
    dotLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 14pt; background: transparent;").arg(dot.name()));
    auto *titleLabel = new QLabel(title, parts.frame);
    titleLabel->setObjectName(QStringLiteral("pTitle"));
    parts.count = new QLabel(parts.frame);
    parts.count->setObjectName(QStringLiteral("pCount"));
    header->addWidget(dotLabel);
    header->addWidget(titleLabel, 1);
    header->addWidget(parts.count);
    parts.layout->addLayout(header);
    return parts;
}

QWidget *personCard(const CloudPerson &person, const QString &subtitle, const CardTone &tone, QWidget *parent)
{
    auto *card = new QFrame(parent);
    card->setObjectName(QStringLiteral("pCard"));
    card->setStyleSheet(QStringLiteral(
                            "QFrame#pCard { background: %1; border-radius: 18px; }"
                            "QLabel#pName { color: %2; font-weight: 700; font-size: 10pt; background: transparent; }"
                            "QLabel#pSub { color: %2; font-size: 8pt; background: transparent; }")
                            .arg(tone.background.name(), tone.foreground.name()));

    auto *row = new QHBoxLayout(card);
    row->setContentsMargins(10, 8, 12, 8);
    row->setSpacing(10);

    auto *avatar = new QLabel(card);
    avatar->setFixedSize(36, 36);
    avatar->setPixmap(Photo::avatar(person.photo, person.name, 36));

    auto *texts = new QVBoxLayout;
    texts->setSpacing(1);
    auto *name = new QLabel(person.name, card);
    name->setObjectName(QStringLiteral("pName"));
    auto *sub = new QLabel(subtitle, card);
    sub->setObjectName(QStringLiteral("pSub"));
    texts->addWidget(name);
    texts->addWidget(sub);

    row->addWidget(avatar);
    row->addLayout(texts, 1);
    return card;
}

}
