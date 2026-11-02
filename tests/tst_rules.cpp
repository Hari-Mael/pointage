#include <QTemporaryDir>
#include <QTest>

#include "auth.h"
#include "database.h"
#include "fields.h"

class TstRules : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void validEmail();
    void validPhone();
    void validCode();
    void normalizeDateFormats();
    void dateOrderEmployee();
    void dateOrderStudentBirthFuture();
    void passwordHashRoundTrip();
    void nextCodeIncrements();
    void marksAndClear();
    void deletePersonRemovesPerson();

private:
    QTemporaryDir m_dir;
    Database m_db;
    int m_userId = 0;
};

void TstRules::initTestCase()
{
    QVERIFY(m_dir.isValid());
    QString error;
    QVERIFY2(m_db.open(m_dir.filePath(QStringLiteral("test.db")), error), qPrintable(error));
    QVERIFY(m_db.createUser(QStringLiteral("a@b.fr"), QStringLiteral("Test"), Auth::hashPassword(QStringLiteral("password1"))));
    m_userId = m_db.findUser(QStringLiteral("a@b.fr"))->id;
}

void TstRules::cleanupTestCase() {}

void TstRules::validEmail()
{
    QVERIFY(isValidEmail(QStringLiteral("jean@exemple.fr")));
    QVERIFY(!isValidEmail(QStringLiteral("jean@exemple")));
    QVERIFY(!isValidEmail(QStringLiteral("jean exemple@x.fr")));
    QVERIFY(!isValidEmail(QStringLiteral("@x.fr")));
}

void TstRules::validPhone()
{
    QVERIFY(isValidPhone(QStringLiteral("+261 34 12 345 67")));
    QVERIFY(!isValidPhone(QStringLiteral("abc")));
    QVERIFY(!isValidPhone(QStringLiteral("123")));
}

void TstRules::validCode()
{
    QVERIFY(isValidCode(QStringLiteral("E001")));
    QVERIFY(isValidCode(QStringLiteral("M-2026_01")));
    QVERIFY(!isValidCode(QStringLiteral("")));
    QVERIFY(!isValidCode(QStringLiteral("ID avec espace")));
    QVERIFY(!isValidCode(QStringLiteral("ABCDEFGHIJKLMNOPQ")));
}

void TstRules::normalizeDateFormats()
{
    QCOMPARE(normalizeDate(QStringLiteral("03/11/2026")), QStringLiteral("2026-11-03"));
    QCOMPARE(normalizeDate(QStringLiteral("2026-11-03")), QStringLiteral("2026-11-03"));
    QVERIFY(normalizeDate(QStringLiteral("31/02/2026")).isEmpty());
    QVERIFY(normalizeDate(QStringLiteral("pas une date")).isEmpty());
}

void TstRules::dateOrderEmployee()
{
    QStringList values(18);
    values[kEmpHireDate] = QStringLiteral("2026-03-01");
    values[kEmpProbationEnd] = QStringLiteral("2026-02-01");
    QVERIFY(!checkDateOrder(kKindEmployee, values).isEmpty());

    values[kEmpProbationEnd] = QStringLiteral("2026-06-01");
    values[kEmpRegularization] = QStringLiteral("2026-05-01");
    QVERIFY(!checkDateOrder(kKindEmployee, values).isEmpty());

    values[kEmpRegularization] = QStringLiteral("2026-07-01");
    QVERIFY(checkDateOrder(kKindEmployee, values).isEmpty());
}

void TstRules::dateOrderStudentBirthFuture()
{
    QStringList values(15);
    values[kStuBirthDate] = QStringLiteral("2999-01-01");
    QVERIFY(!checkDateOrder(kKindStudent, values).isEmpty());
    values[kStuBirthDate] = QStringLiteral("2010-01-01");
    QVERIFY(checkDateOrder(kKindStudent, values).isEmpty());
}

void TstRules::passwordHashRoundTrip()
{
    const QString hash = Auth::hashPassword(QStringLiteral("secret123"));
    QVERIFY(Auth::verifyPassword(QStringLiteral("secret123"), hash));
    QVERIFY(!Auth::verifyPassword(QStringLiteral("secret124"), hash));
    QVERIFY(!Auth::verifyPassword(QStringLiteral("secret123"), QStringLiteral("n_importe_quoi")));
    QVERIFY(Auth::selfTest());
}

void TstRules::nextCodeIncrements()
{
    QCOMPARE(m_db.nextCode(m_userId, kKindEmployee, QLatin1Char('E')), QStringLiteral("E001"));
    PersonRecord p;
    p.code = QStringLiteral("E001");
    p.name = QStringLiteral("Jean Dupont");
    p.fields = QStringList(18);
    QVERIFY(m_db.addPerson(m_userId, kKindEmployee, p));
    QCOMPARE(m_db.nextCode(m_userId, kKindEmployee, QLatin1Char('E')), QStringLiteral("E002"));
}

void TstRules::marksAndClear()
{
    PersonRecord p;
    p.code = QStringLiteral("E050");
    p.name = QStringLiteral("Marie Test");
    p.fields = QStringList(18);
    QVERIFY(m_db.addPerson(m_userId, kKindEmployee, p));
    const int id = [&] {
        for (const PersonRecord &x : m_db.persons(m_userId, kKindEmployee))
            if (x.code == QLatin1String("E050"))
                return x.id;
        return 0;
    }();
    QVERIFY(id > 0);

    QVERIFY(m_db.setMark(id, QDate(2026, 11, 2), QLatin1Char('P')));
    QVERIFY(m_db.setMark(id, QDate(2026, 11, 3), QLatin1Char('A')));
    QVERIFY(m_db.setMark(id, QDate(2026, 12, 1), QLatin1Char('P')));
    QCOMPARE(m_db.marks(id, 2026, 11).size(), 2);

    QVERIFY(m_db.setMark(id, QDate(2026, 11, 3), QLatin1Char('P')));
    QCOMPARE(m_db.marks(id, 2026, 11).value(3), QLatin1Char('P'));

    QVERIFY(m_db.setMark(id, QDate(2026, 11, 2), QChar()));
    QCOMPARE(m_db.marks(id, 2026, 11).size(), 1);

    QVERIFY(m_db.clearMarks(id));
    QCOMPARE(m_db.marks(id, 2026, 11).size(), 0);
    QCOMPARE(m_db.marks(id, 2026, 12).size(), 0);
}

void TstRules::deletePersonRemovesPerson()
{
    PersonRecord p;
    p.code = QStringLiteral("E090");
    p.name = QStringLiteral("A Supprimer");
    p.fields = QStringList(18);
    QVERIFY(m_db.addPerson(m_userId, kKindEmployee, p));
    int id = 0;
    for (const PersonRecord &x : m_db.persons(m_userId, kKindEmployee))
        if (x.code == QLatin1String("E090"))
            id = x.id;
    QVERIFY(id > 0);
    QVERIFY(m_db.setMark(id, QDate(2026, 11, 4), QLatin1Char('A')));
    QVERIFY(m_db.deletePerson(id));
    for (const PersonRecord &x : m_db.persons(m_userId, kKindEmployee))
        QVERIFY(x.code != QLatin1String("E090"));
    QCOMPARE(m_db.marks(id, 2026, 11).size(), 0);
}

QTEST_MAIN(TstRules)
#include "tst_rules.moc"
