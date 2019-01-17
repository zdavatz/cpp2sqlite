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
    std::cout << "SQLITE_VERSION: " << SQLITE_VERSION << std::endl;  // 3.24.0
    //std::cout << "SQLITE_VERSION_NUMBER: " << SQLITE_VERSION_NUMBER << std::endl; // 3024000
    //std::cout << "sqlite3_libversion: " << sqlite3_libversion() << std::endl;  // 3.24.0

    AIPS::MedicineList &list = AIPS::parseXML(argv[1]);
    std::cout << "title count: " << list.size() << std::endl;  // 22056

    sqlite3 *db = AIPS::createDB("amiko_db_full_idx_de.db");

    std::cerr << "Populating amiko_db_full_idx_de.db..." << std::endl;
    for (AIPS::Medicine m : list) {
        std::ostringstream values;
        values << "null, '" << m.title << "'";
        AIPS::insertInto("amikodb", "_id, title", values.str()); // SqlDatabase.java:222
    }

    int rc = sqlite3_close(db);
    if (rc != SQLITE_OK)
        std::cout << basename((char *)__FILE__) << ":" << __LINE__ << ", rc" << rc << std::endl;

    return EXIT_SUCCESS;
}
