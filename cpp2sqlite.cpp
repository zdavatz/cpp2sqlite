/* Alessandro Bettarini - 15 Jan 2019
 */

#include <iostream>
#include <sstream>
#include <string>
#include <sqlite3.h>
#include <libgen.h>     // for basename()

#include "aips.hpp"
#include "sqlDatabase.hpp"

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::cerr << "Specify full path of aips_xml.xml" << std::endl;
        return EXIT_FAILURE;
    }

    std::cerr << basename(argv[0]) << " " << __DATE__ << " " << __TIME__ << std::endl;
    std::cerr << "C++ " << __cplusplus << std::endl;
    std::cout << "SQLITE_VERSION: " << SQLITE_VERSION << std::endl;

    AIPS::MedicineList &list = AIPS::parseXML(argv[1]);
    std::cout << "title count: " << list.size() << std::endl;

    sqlite3 *db = AIPS::createDB("amiko_db_full_idx_de.db");

    sqlite3_stmt *statement;
    AIPS::prepareStatement("amikodb", &statement,
                           "null, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?");

    std::cerr << "Populating amiko_db_full_idx_de.db" << std::endl;
    for (AIPS::Medicine m : list) {
        AIPS::bindText("amikodb", statement, 1, m.title);
        AIPS::bindText("amikodb", statement, 2, m.auth);
        AIPS::bindText("amikodb", statement, 4, m.subst);
        // TODO: add all other columns

        AIPS::runStatement("amikodb", statement);
    }

    AIPS::destroyStatement(statement);

    int rc = sqlite3_close(db);
    if (rc != SQLITE_OK)
        std::cerr << basename((char *)__FILE__) << ":" << __LINE__ << ", rc" << rc << std::endl;

    return EXIT_SUCCESS;
}
