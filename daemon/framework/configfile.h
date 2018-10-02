#ifndef CONFIGFILE_H
#define CONFIGFILE_H

#include <common_defs.h>

#include <string>

#include "configdata.h"

namespace FrameworkLibrary {

/** @brief Read a configuration file
 *
 * This class can parse a configuration file and create the hierarchy of ConfigData classes which maps it. This class
 * itself is based from ConfigData, which means that a configuration file can have rows outside any section.
 *
 * Reading Simulation Framework Configuration Files
 * ================================================
 *
 * This is the intended way to use this class. Just call FrameworkInterface::getConfigFile() and use the returned object.
 * Please remember to DELETE the returned pointer, you are it's owner.
 *
 * Reading any other file
 * ======================
 *
 * if you want to use it directly, for example on your own data stream or a local configuration file, you can do so.
 * In this case it's up to you to read the entire data to a string and then feed it to the constructor.
 * In this case, the configuration file name is optional and used only for your own reference.
 *
 * Comments in configuration files
 * ===============================
 *
 * Comments are supported, starting with the "#" character and can be accessed like any other row or section. A comment is
 * always associated to a row or a section. See ConfigData for more details.
 *
 * Configuration file format
 * =========================
 *
 * A typical configuration file looks like:
 * @code
 * outside_row = value1
 * outside_row2 = my next value
 *
 * [section 1]
 * row1 = value1
 * # comment for row2
 * row2 = value2
 *
 * # Comments for this sub-section
 * [section 1:sub_section_1]
 * rowA = my row sub value
 *
 * [section 2]
 * rowA = valueA
 *      .....
 * @endcode
 *
 *
 * @include example_cpp_read_config.cpp
 * @include example_cpp_write_config.cpp
 *
 * 
 *
 */
class DLLEXPORT ConfigFile:
        public ConfigData
{
public:
    /** @brief Load a configuration file from string
     * @param name Name of the configuration file
     * @param file_data string containing the configuration file data
     *
     * 
     *
     */
    ConfigFile( std::string name, std::string file_data );
    ConfigFile( std::string filename );
    virtual ~ConfigFile();


    /** @brief get the configuration file as a string, so it can be saved to disk
     *
     * The string contains all the proper newlines and format, just write it to a file.
     * @return ASCII string of the configuration file.
     *
     * 
     *
     */
    std::string toStr() const;

    bool saveToFile();

private:
    void initialize(std::string file_data );
};

}

#endif // CONFIGFILE_H
