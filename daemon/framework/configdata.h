#ifndef CONFIGDATA_H
#define CONFIGDATA_H

#include <common_defs.h>

#include <string>
#include <map>
#include <list>
#include <vector>

#include "frameworkutils.h"

namespace FrameworkLibrary {

class __private_ConfigData;
/**
 * @brief Manage a configuration section in a configuration file
 *
 * This is the base class to manage configuration files. This class contains all the methods to access and modify contents
 * of a config file.
 *
 * Reading Configuration Files
 * ===========================
 *
 * Configuration files must be read by creating an instance of the ConfigFile class, please check it's documentation for more information.
 *
 * Configuration file format
 * =========================
 *
 * This class manages a section made of rows, each row is a pair of name = value. Each section can have zero or more sub-sections,
 * there is no depth limit to subsections.
 *
 * The configuration file format is as follow:
 * For example:
 * @code
 * # comment 1
 * # comment 2
 * [test]
 * name = Pete
 * # he is old!
 * age = 31
 * # Valid jobs:
 * # - sales
 * # - accounting
 * job = sales
 *
 * [test:sub_section]
 * other = one
 * more = two
 *
 * [test:sub_section:more_sub_section]
 * one_more = seven
 * eye_color = green
 * @endcode
 *
 * this will create a thre-level section/subsection/sub-subsection.
 *
 * This class allow to manage comments too, each comment line shall be associated to a specific row, you can have as many comment lines
 * for each row.
 *
 * You can have comments for the entire section or for a single row.
 *
 * Usage examples
 * ==============
 *
 * Please check ConfigFile class for usage examples.
 *
 *
 * 
 * 
 *
 */
class DLLEXPORT ConfigData
{
public:
    virtual ~ConfigData();

    /** @brief get name of the section
     * @return name of the section
     *
     * 
     *
     */
    std::string getName() const;

    /** @brief get FULL name of the section (complete of all parents names)
     * @since 3.4
     * @return name of the section, complete with all parents names
     *
     * 
     *
     */
    std::string getFullName() const;

    /** @brief set name of the section
     * @param nn new name of the section
     *
     * 
     *
     */
    void setName( const std::string& nn );

    /** @brief get the parent section for this section
     * @return NULL if there is no parent section, or pointer to the parent section
     *
     * 
     *
     */
    ConfigData* getParent();

    /** @brief check if a session is modified
     * @return true if the section has been modified, false otherwise.
     *
     * 
     *
     */
    bool isModified() const;

    /** @brief Return true if the section has no rows and no subsections
     * @since 3.4
     * @return return true if section has no rows or sub-sections, false otherwise.
     *
     * 
     *
     */
    bool isEmpty() const;

    /** @brief Convert the section to a string, for saving and transmitting.
     * @return the section contents, as a string.
     *
     * 
     *
     */
    virtual std::string toStr() const;

public: // Manage subsections, read only
    /** @brief Return number of subsections
     * @since 3.4
     * @return number of subsections
     *
     * 
     *
     */
    uint32_t getNumSections() const;


    /** @brief list all the sub-sections by names
     * @since 3.4
     * @return list of sub-sections
     *
     * 
     *
     */
    std::list<ConfigData *> listSections() const;

    /** @brief get a subsection by name
     * @since 4.0.0 added the "seq" parameter
     *
     * In case of multiple sections with the same name, you can access them by specifiying the sequential number.
     * WIth 0 you can access the first one, and so on. If you specify a number too high, NULL will be returned.
     * For example, if you have a section called "TEST" repeated twice, you can access the first copy with seq = 0
     * and the second copy with seq = 1. If you specify seq >= 2, NULL will be returned.
     * @param name Name of subsection to get
     * @param seq in case of multiple sections with the same name, specify which one you want to get.
     * @return the subsection
     *
     * 
     *
     */
    ConfigData* getSection( const std::string& name, uint32_t seq = 0 );

    /** @brief get number of sections with the same name
     * @since 4.0.0
     * @param name name of section
     * @return multeplicity of the given section, from 0 (=does not exist)
     */
    uint32_t getSectionMulteplicity( const std::string& name );


public: // Manage subsections, write
    /** @brief create a subsection to the section
     * @warning This section will own the new sub-section, do not delete it yourself!
     * @param name Sub-section name to create
     * @return pointer to the subsection, NULL if it already exist.
     *
     * 
     *
     */
    ConfigData* newSection( const std::string& name );

    /** @brief Delete a subsection to the section
     * @warning If returned true, the subsection has been deleted, don't use the pointer!
     * @param subsection Sub-section name to delete
     * @return true if the subsection has been deleted, false if the subsection does not belog to this section.
     */
    bool delSection( ConfigData* subsection );

    /** @brief delete all sections
     * @since 4.0.3
     */
    void clearSections();

public: // Manage values - methods avilable for reader only
    /** @brief return a list (ordered) of the rows of the section
     * @since 3.3
     * @return list containing all the names of all the rows (all names will be lower case!) in the same order they appear in the file
     *
     * 
     *
     */
    std::list<std::string> listNames() const;

    /** @brief Check if a row exists
     * @param what name of the row to look for
     * @return true if the row exists, false otherwise.
     *
     * 
     *
     */
    bool hasValue( const std::string& what ) const;

    /** @brief Get list of values. The list can be refined by prefix
     *
     * This function return a map of names/values. The map can be filtered by prefix, and the prefix will be
     * removed from the names. For example, if you have the following rows:
     * row_first = xxx
     * column_other = yyy
     * Passing prefix = "row_" will result in selecting only "row_first" and removing the "row_" from it, thus the return will be only:
     * first = xxx
     *
     * @param prefix prefix to use (leave "" for none)
     * @return map of names and values. The names will have the prefix removed.
     * @since 3.3
     *
     * 
     *
     */
    std::map<std::string, std::string> getValues( const std::string& prefix = "" ) const;

    /** @brief Get the value of a given row
     * @warning Since 3.4
     * @param what name of the row
     * @return value of the row
     *
     * 
     *
     */
    std::string getValue( const std::string& what ) const;

    /** @brief Get the value of a given row as a boolean
     *
     * Will return true if the row value is: 1, true, yes, si. Will return false in all other cases.
     * It is case-insensitive.
     * @param what name of the row
     * @return true or false
     *
     * 
     *
     */
    bool getValueBool( const std::string& what ) const;


public: // Values - modify content
    /** @brief Adds a new row to the section
     *
     * If the row already exist, it will overwrite it (identical to setValue())
     * @param what name of the row
     * @param value value of the row
     *
     * 
     *
     */
    void addValue( const std::string& what, const std::string& value );

    /** @brief Set a row with a value to the section
     *
     * If the row do not exist, it will be created (identical to addValue())
     * @param what name of the row
     * @param value value of the row
     *
     * 
     *
     */
    void setValue( const std::string& what, const std::string& value );

    /** @brief Set a row with a boolean value to the section
     * @since 4.0.3
     * If the row do not exist, it will be created (identical to addValue())
     * @param what name of the row
     * @param value value of the row
     */
    void setValueBool( const std::string& what, bool value );

    /** @brief Remove a row
     * @param what name of the row to remove
     *
     * 
     *
     */
    void removeValue( const std::string& what );

    /** @brief Remove all values
     * @since 3.4
     *
     * 
     *
     */
    void clearValues();


public: // comments, read only
    /** @brief Return all comments
     * @since 3.3
     * @param what row to get comments for (leave empty for section comments)
     * @return list of comments rows (the starting # will be removed automatically)
     *
     * 
     *
     */
    std::vector<std::string> getComments( const std::string& what ) const;


public: // comments, write
    /** @brief Remove all comments
     * @since 3.3
     * @param what row to clear comments (leave empty for section comments)
     *
     * 
     *
     */
    void clearComments( const std::string& what );

    /** @brief Set comments (will replace existing comments)
     * @since 3.3
     * @param what row to get comments for (leave empty for section comments)
     * @param new_comments output list of comments rows (the starting # will be removed automatically)
     *
     * 
     *
     */
    void setComments(const std::string& what, const std::vector<std::string>& new_comments );

    /** @brief Reset the "modified" flag, used while loading the section from disk
     *
     * 
     *
     */
    void resetModified();

protected:
    /** @brief Create a new section
     * @param n the name
     * @param parent the parent section
     *
     * 
     * 
     *
     */
    ConfigData(const std::string& n, ConfigData* parent);

    /** @brief internal, write the section header part
     * @return the header part
     *
     * 
     *
     */
    std::string _writeHeader() const;

    /** @brief internal, write the section values part
     * @return the values part
     *
     * 
     *
     */
    std::string _writeValues() const;

    /** @brief internal, write the section subsections part
     * @return the sunsections part
     *
     * 
     *
     */
    std::string _writeSubSections() const;

private:
    __private_ConfigData* _private;
};

}

#endif // CONFIGDATA_H
