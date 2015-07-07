//******************************************************************
//
// Copyright 2015 Samsung Electronics All Rights Reserved.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Configuration.h"

namespace OIC
{
    namespace Service
    {
        static inline std::string trim_both(const std::string &str)
        {
            int npos = str.find_first_not_of(" \t\v\n\r");

            if (npos == -1)
            {
                return "";
            }

            unsigned int n = (unsigned int) npos;
            std::string tempString = n == std::string::npos ? str : str.substr(n, str.length());

            n = tempString.find_last_not_of(" \t\v\n\r");

            return n == std::string::npos ? tempString : tempString.substr(0, n + 1);
        }

        Configuration::Configuration()
        {
            m_loaded = false;
        }

        Configuration::Configuration(string configFile)
        {
            m_loaded = false;

            getCurrentPath(&m_pathConfigFile);
            m_pathConfigFile.append("/");
            m_pathConfigFile.append(configFile);

            getConfigDocument(m_pathConfigFile);
        }

        Configuration::~Configuration()
        {
        }

        bool Configuration::isLoaded()
        {
            return m_loaded;
        }

        void Configuration::getConfiguredBundles(configInfo *configOutput)
        {
            rapidxml::xml_node< char > *bundle;
            rapidxml::xml_node< char > *subItem;

            string strKey, strValue;

            try
            {
                //cout << "Name of first node is: " << m_xmlDoc.first_node()->name() << endl;

                for (bundle = m_xmlDoc.first_node()->first_node("bundle"); bundle; bundle = bundle->next_sibling())
                {
                    std::map< std::string, std::string > bundleMap;
                    //cout << "Bundle: " << bundle->name() << endl;
                    for (subItem = bundle->first_node(); subItem; subItem = subItem->next_sibling())
                    {
                        strKey = subItem->name();
                        strValue = subItem->value();

                        if (strlen(subItem->value()) > 0)
                        {
                            bundleMap.insert(std::make_pair(trim_both(strKey), trim_both(strValue)));
                            //cout << strKey << " " << strValue << endl;
                        }
                    }
                    configOutput->push_back(bundleMap);
                }

            }
            catch (rapidxml::parse_error &e)
            {
                cout << "xml parsing failed !!" << endl;
                cout << e.what() << endl;
            }
        }

        void Configuration::getBundleConfiguration(string bundleId, configInfo *configOutput)
        {
            rapidxml::xml_node< char > *bundle;

            string strBundleId, strPath, strVersion;

            try
            {
                std::map< std::string, std::string > bundleConfigMap;

                // <bundle>
                for (bundle = m_xmlDoc.first_node()->first_node("bundle"); bundle; bundle = bundle->next_sibling())
                {
                    // <id>
                    strBundleId = bundle->first_node("id")->value();

                    if (!strBundleId.compare(bundleId))
                    {
                        bundleConfigMap.insert(std::make_pair("id", trim_both(strBundleId)));

                        // <path>
                        strPath = bundle->first_node("path")->value();
                        bundleConfigMap.insert(std::make_pair("path", trim_both(strPath)));

                        // <version>
                        strVersion = bundle->first_node("version")->value();
                        bundleConfigMap.insert(std::make_pair("version", trim_both(strVersion)));

                        configOutput->push_back(bundleConfigMap);

                        break;
                    }
                }
            }
            catch (rapidxml::parse_error &e)
            {
                cout << "xml parsing failed !!" << endl;
                cout << e.what() << endl;
            }
        }

        void Configuration::getResourceConfiguration(std::string bundleId,
                std::vector< resourceInfo > *configOutput)
        {
            rapidxml::xml_node< char > *bundle;
            rapidxml::xml_node< char > *resource;
            rapidxml::xml_node< char > *item, *subItem, *subItem2;

            string strBundleId;
            string strKey, strValue;

            try
            {
                // <bundle>
                for (bundle = m_xmlDoc.first_node()->first_node("bundle"); bundle; bundle = bundle->next_sibling())
                {
                    // <id>
                    strBundleId = bundle->first_node("id")->value();

                    if (!strBundleId.compare(bundleId))
                    {
                        // <resourceInfo>
                        for (resource = bundle->first_node("resources")->first_node("resourceInfo"); resource;
                             resource = resource->next_sibling())
                        {
                            resourceInfo tempResourceInfo;

                            for (item = resource->first_node(); item; item = item->next_sibling())
                            {
                                strKey = item->name();
                                strValue = item->value();

                                if (!strKey.compare("name"))
                                    tempResourceInfo.name = trim_both(strValue);

                                else if (!strKey.compare("uri"))
                                    tempResourceInfo.uri = trim_both(strValue);

                                else if (!strKey.compare("address"))
                                    tempResourceInfo.address = trim_both(strValue);

                                else if (!strKey.compare("resourceType"))
                                    tempResourceInfo.resourceType = trim_both(strValue);

                                else
                                {
                                    for (subItem = item->first_node(); subItem; subItem = subItem->next_sibling())
                                    {
                                        map< string, string > propertyMap;

                                        strKey = subItem->name();

                                        for (subItem2 = subItem->first_node(); subItem2; subItem2 = subItem2->next_sibling())
                                        {
                                            string newStrKey = subItem2->name();
                                            string newStrValue = subItem2->value();

                                            propertyMap[trim_both(newStrKey)] = trim_both(newStrValue);
                                        }

                                        tempResourceInfo.resourceProperty[trim_both(strKey)].push_back(propertyMap);
                                    }
                                }
                            }
                            configOutput->push_back(tempResourceInfo);
                        }
                    }
                }
            }
            catch (rapidxml::parse_error &e)
            {
                std::cout << "xml parsing failed !!" << std::endl;
                std::cout << e.what() << std::endl;
            }
        }

        void Configuration::getConfigDocument(std::string pathConfigFile)
        {
            std::basic_ifstream< char > xmlFile(pathConfigFile.c_str());

            if (!xmlFile.fail())
            {
                xmlFile.seekg(0, std::ios::end);
                unsigned int size = (unsigned int) xmlFile.tellg();
                xmlFile.seekg(0);

                std::vector< char > xmlData(size + 1);
                xmlData[size] = 0;

                xmlFile.read(&xmlData.front(), (std::streamsize) size);
                xmlFile.close();
                m_strConfigData = std::string(xmlData.data());

                try
                {
                    m_xmlDoc.parse< 0 >((char *)m_strConfigData.c_str());
                    m_loaded = true;
                }
                catch (rapidxml::parse_error &e)
                {
                    std::cout << "xml parsing failed !!" << std::endl;
                    std::cout << e.what() << std::endl;
                }
            }
            else
            {
                std::cout << "Configuration File load failed !!" << std::endl;
            }
        }

        void Configuration::getCurrentPath(std::string *pPath)
        {
            char buffer[2048];
            char *strPath = NULL;

            int length = readlink("/proc/self/exe", buffer, 2047);

            buffer[length] = '\0';

            strPath = strrchr(buffer, '/');

            *strPath = '\0';

            pPath->append(buffer);
        }
    }
}
