#include <QtTest>

class LandropTest : public QObject
{
    Q_OBJECT

  public:
    LandropTest();
    ~LandropTest();

  private slots:
    void test_case();
};

LandropTest::LandropTest()
{
}

LandropTest::~LandropTest()
{
}

void LandropTest::test_case()
{
    QVERIFY(true);
}

QTEST_APPLESS_MAIN(LandropTest)

#include "test_main.moc"

