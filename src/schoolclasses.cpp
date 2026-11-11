#include "schoolclasses.h"

namespace SchoolClasses {

QStringList all()
{
    return {
        QStringLiteral("5ème 1"), QStringLiteral("5ème 2"), QStringLiteral("5ème 3"),
        QStringLiteral("4ème 1"), QStringLiteral("4ème 2"), QStringLiteral("4ème 3"),
        QStringLiteral("2nde 1"), QStringLiteral("2nde 2"), QStringLiteral("2nde 3"), QStringLiteral("2nde 4"),
        QStringLiteral("1ère L1"), QStringLiteral("1ère L2"),
        QStringLiteral("1ère S1"), QStringLiteral("1ère S2"),
    };
}

}
