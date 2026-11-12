#include <QJsonArray>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>

#include "localdb.h"

class TstLocalDb : public QObject {
    Q_OBJECT

private slots:
    void storesPersonsAndQueuesChanges();
    void marksAndSettingsAreQueued();
    void accountsAreIsolated();
    void syncedOperationsLeaveTheQueue();
};

namespace {

QJsonObject samplePerson(const QString &id, const QString &code)
{
    return QJsonObject{{QStringLiteral("id"), id},
                       {QStringLiteral("kind"), 0},
                       {QStringLiteral("code"), code},
                       {QStringLiteral("name"), QStringLiteral("Nom %1").arg(code)},
                       {QStringLiteral("class_name"), QString()},
                       {QStringLiteral("photo"), QString()},
                       {QStringLiteral("fields"), QJsonArray{QStringLiteral("Jean")}}};
}

}

void TstLocalDb::storesPersonsAndQueuesChanges()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    LocalDb db;
    QString error;
    QVERIFY2(db.open(dir.path(), QStringLiteral("userA"), error), qPrintable(error));

    QVERIFY(db.upsertPerson(samplePerson(QStringLiteral("p1"), QStringLiteral("E001"))));
    QCOMPARE(db.persons().size(), 1);
    QCOMPARE(db.pendingCount(), 1);
    QVERIFY(db.deletePerson(QStringLiteral("p1")));
    QCOMPARE(db.persons().size(), 0);
    QCOMPARE(db.pendingCount(), 2);
}

void TstLocalDb::marksAndSettingsAreQueued()
{
    QTemporaryDir dir;
    LocalDb db;
    QString error;
    QVERIFY2(db.open(dir.path(), QStringLiteral("userB"), error), qPrintable(error));

    QVERIFY(db.setMark(QStringLiteral("p1"), QDate(2026, 11, 3), QChar('P')));
    QVERIFY(db.setMark(QStringLiteral("p1"), QDate(2026, 11, 4), QChar('R')));
    QCOMPARE(db.marksOf(QStringLiteral("p1")).value(QStringLiteral("3")).toString(), QStringLiteral("P"));
    QVERIFY(db.setMark(QStringLiteral("p1"), QDate(2026, 11, 3), QChar()));
    QCOMPARE(db.marksOf(QStringLiteral("p1")).contains(QStringLiteral("3")), false);
    QVERIFY(db.setRestMask(65));
    QCOMPARE(db.restMask(), 65);
    QCOMPARE(db.pendingCount(), 4);

    const QList<LocalOp> ops = db.pendingOps();
    QCOMPARE(ops.size(), 4);
    QCOMPARE(ops.first().kind, QStringLiteral("mark.set"));
    QCOMPARE(ops.last().kind, QStringLiteral("settings.rest"));
}

void TstLocalDb::accountsAreIsolated()
{
    QTemporaryDir dir;
    LocalDb a;
    LocalDb b;
    QString error;
    QVERIFY2(a.open(dir.path(), QStringLiteral("userA"), error), qPrintable(error));
    QVERIFY2(b.open(dir.path(), QStringLiteral("userB"), error), qPrintable(error));

    QVERIFY(a.upsertPerson(samplePerson(QStringLiteral("p1"), QStringLiteral("E018"))));
    QCOMPARE(a.persons().size(), 1);
    QCOMPARE(b.persons().size(), 0);
    QCOMPARE(b.pendingCount(), 0);
}

void TstLocalDb::syncedOperationsLeaveTheQueue()
{
    QTemporaryDir dir;
    LocalDb db;
    QString error;
    QVERIFY2(db.open(dir.path(), QStringLiteral("userC"), error), qPrintable(error));

    QVERIFY(db.upsertPerson(samplePerson(QStringLiteral("p1"), QStringLiteral("E001"))));
    QVERIFY(db.setAvatar(QStringLiteral("abc")));
    QCOMPARE(db.pendingCount(), 2);

    const QList<LocalOp> ops = db.pendingOps();
    QVERIFY(db.markSynced(ops.first().seq));
    QCOMPARE(db.pendingCount(), 1);
    QCOMPARE(db.avatar(), QStringLiteral("abc"));
}

QTEST_MAIN(TstLocalDb)
#include "tst_localdb.moc"
