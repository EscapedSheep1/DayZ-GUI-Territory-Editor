#include "TerritoryParser.h"
#include "tinyxml2.h"
#include <algorithm>
#include <sstream>

using namespace tinyxml2;

bool TerritoryParser::LoadFromFile(const std::string& filepath, TerritoryData& data) {
    data.clear();
    
    XMLDocument doc;
    if (doc.LoadFile(filepath.c_str()) != XML_SUCCESS) {
        return false;
    }
    
    XMLElement* root = doc.FirstChildElement("territory-type");
    if (!root) {
        return false;
    }
    
    XMLElement* territoryElem = root->FirstChildElement("territory");
    int territoryIndex = 0;
    
    while (territoryElem) {
        Territory territory;
        
        // Get color attribute
        const char* colorStr = territoryElem->Attribute("color");
        if (colorStr) {
            territory.color = static_cast<uint32_t>(std::stoul(colorStr));
        }
        
        // Get name attribute if present
        const char* nameStr = territoryElem->Attribute("name");
        if (nameStr) {
            territory.name = nameStr;
        }
        
        // Parse zones
        XMLElement* zoneElem = territoryElem->FirstChildElement("zone");
        while (zoneElem) {
            Zone zone;
            
            const char* name = zoneElem->Attribute("name");
            if (name) zone.name = name;
            
            zoneElem->QueryIntAttribute("smin", &zone.smin);
            zoneElem->QueryIntAttribute("smax", &zone.smax);
            zoneElem->QueryIntAttribute("dmin", &zone.dmin);
            zoneElem->QueryIntAttribute("dmax", &zone.dmax);
            zoneElem->QueryFloatAttribute("x", &zone.x);
            zoneElem->QueryFloatAttribute("z", &zone.z);
            zoneElem->QueryFloatAttribute("r", &zone.r);
            zoneElem->QueryFloatAttribute("h", &zone.h);
            
            territory.zones.push_back(zone);
            zoneElem = zoneElem->NextSiblingElement("zone");
        }
        
        if (!territory.zones.empty()) {
            // If no name was set, use the name from the first zone
            if (territory.name.empty() && !territory.zones[0].name.empty()) {
                territory.name = territory.zones[0].name;
            } else if (territory.name.empty()) {
                territory.name = "Territory " + std::to_string(territoryIndex);
            }
            data.territories.push_back(territory);
        }
        
        territoryElem = territoryElem->NextSiblingElement("territory");
        territoryIndex++;
    }
    
    return true;
}

bool TerritoryParser::SaveToFile(const std::string& filepath, const TerritoryData& data) {
    XMLDocument doc;
    doc.InsertFirstChild(doc.NewDeclaration());
    
    XMLElement* root = doc.NewElement("territory-type");
    doc.InsertEndChild(root);
    
    for (const auto& territory : data.territories) {
        XMLElement* territoryElem = doc.NewElement("territory");
        territoryElem->SetAttribute("color", static_cast<int64_t>(territory.color));
        
        if (!territory.name.empty()) {
            territoryElem->SetAttribute("name", territory.name.c_str());
        }
        
        for (const auto& zone : territory.zones) {
            XMLElement* zoneElem = doc.NewElement("zone");
            zoneElem->SetAttribute("name", zone.name.c_str());
            zoneElem->SetAttribute("smin", zone.smin);
            zoneElem->SetAttribute("smax", zone.smax);
            zoneElem->SetAttribute("dmin", zone.dmin);
            zoneElem->SetAttribute("dmax", zone.dmax);
            zoneElem->SetAttribute("x", zone.x);
            zoneElem->SetAttribute("z", zone.z);
            zoneElem->SetAttribute("r", zone.r);
            if (zone.h != 0.0f) {
                zoneElem->SetAttribute("h", zone.h);
            }
            
            territoryElem->InsertEndChild(zoneElem);
        }
        
        root->InsertEndChild(territoryElem);
    }
    
    return doc.SaveFile(filepath.c_str()) == XML_SUCCESS;
}

std::string TerritoryParser::ExtractTerritoryName(const Territory& territory) {
    if (!territory.zones.empty()) {
        return territory.zones[0].name;
    }
    return "Unknown";
}

