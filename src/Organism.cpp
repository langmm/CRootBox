// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
#include "Organism.h"

#include "OrganParameter.h"
#include "Organ.h"

#include <stdexcept>
#include <iostream>
#include <ctime>
#include <numeric>

namespace CRootBox {

std::vector<std::string> Organism::organTypeNames = { "organ", "seed", "root", "stem", "leaf" };

/**
 * @return the organ type number of an organ type name @param name
 */
int Organism::organTypeNumber(std::string name)
{
    int ot = 0;
    try {
        while (name.compare(organTypeNames.at(ot))!=0) {
            ot++;
        }
    } catch (const std::exception& e) {
        std::cout << "Organism::organTypeNumber: unknown organ type name " << name << "\n";
        throw(e);
    }
    return ot;
}

/**
 * @return the organ type name of an organ type number @param ot
 */
std::string Organism::organTypeName(int ot)
{
    try {
        return organTypeNames.at(ot);
    } catch (const std::exception& e) {
        std::cout << "Organism::organTypeName: unknown organ type number " << ot << "\n";
        throw(e);
    }

}

/**
 * Copy constructor
 */
Organism::Organism(const Organism& o): organParam(o.organParam), simtime(o.simtime),
    organId(o.organId), nodeId(o.nodeId), gen(o.gen), UD(o.UD), ND(o.ND)
{
    // std::cout << "Copying organism with "<<o.baseOrgans.size()<< " base organs \n";
    baseOrgans.resize(o.baseOrgans.size());  // copy base organs
    for (int i=0; i<baseOrgans.size(); i++) {
        baseOrgans[i] = o.baseOrgans[i]->copy(this);
    }
    for (int ot = 0; ot < numberOfOrganTypes; ot++) { // copy organ type parameters
        for (auto& otp : organParam[ot]) {
            otp.second = otp.second->copy(this);
        }
    }
}

/*
 * Destructor: deletes all base organs, and organ type parameters
 */
Organism::~Organism()
{
    for(auto o :baseOrgans) { // delete base organs
        delete o;
    }
    for (int ot = 0; ot < numberOfOrganTypes; ot++) {  // delete organ type parameters
        for (auto& otp : organParam[ot]) {
            delete otp.second;
        }
    }
}

/**
 * Copies the organ type parameters of a specific organ type into a vector
 *
 * @param ot    the organ type
 */
std::vector<OrganTypeParameter*> Organism::getOrganTypeParameter(int ot) const
{
    std::vector<OrganTypeParameter*>  otps = std::vector<OrganTypeParameter*>(0);
    for (auto& otp : organParam[ot]) {
        otps.push_back(otp.second);
    }
    return otps;
}

/**
 * Returns an organ type parameter of a specific organ type and sub type
 *
 * @param ot       the organ type (e.g. ot_root)
 * @param subType  the sub type (e.g. root type)
 * @return         the respective type parameter
 */
OrganTypeParameter* Organism::getOrganTypeParameter(int ot, int subtype) const
{
    try {
//                std::cout << "reading organ type " << ot << " sub type " << subtype <<": ";
//                for (auto& p : organParam[ot]) {
//                    std::cout << p.first;
//                }
//                std::cout << "\n" << std::flush;
        return organParam[ot].at(subtype);
    } catch(const std::out_of_range& oor) {
        std::cout << "Organism::getOrganTypeParameter: Organ type parameter of sub type " << subtype << " was not set \n" << std::flush;
        throw;
    }
}

/**
 *  Sets the  type parameter, subType and organType defined within p
 *  Deletes the old parameter if there is one, takes ownership of the new one
 *
 *  @param p    the organ type parameter
 */
void Organism::setOrganTypeParameter(OrganTypeParameter* p)
{
    assert(p->plant == this && "OrganTypeParameter::plant should be this organism");
    int otype = p->organType;
    int subtype = p->subType;
    try {
        delete organParam[otype][subtype];
    } catch (std::exception& e) {
        // did not exist, nothing to delete
    }
    organParam[otype][subtype] = p;
    // std::cout << "setting organ type " << otype << " sub type " << subtype << "\n";
}

/**
 * Overwrite if there is the need for additional initializations,
 * before simulation starts.
 *
 * e.g. initialization of GrowthFunctions, TropismFunctions, set up base Organs
 */
void Organism::initialize()
{ }

/**
 * Simulates the development of the organism in a time span of @param dt days.
 *
 * @param dt        time step [day]
 * @param verbose   turns console output on or off
 */
void Organism::simulate(double dt, bool verbose)
{
    if (verbose) {
        std::cout << "Organism::simulate: from "<< simtime << " to " << simtime+dt << " days" << std::endl;
    }
    oldNumberOfNodes = getNumberOfNodes();
    oldNumberOfOrgans = getNumberOfOrgans();
    for (const auto& r : baseOrgans) {
        r->simulate(dt, verbose);
    }
    simtime+=dt;
}

/**
 * Creates a sequential list of organs. Considers only organs with more than 1 node.
 *
 * @param ot        the expected organ type, where -1 denotes all organ types (default).
 * @return Sequential list of organs. If there is less than one node,
 * or another organ type is expected, an empty vector is returned.
 */
std::vector<Organ*> Organism::getOrgans(int ot) const
{
    std::vector<Organ*> organs = std::vector<Organ*>(0);
    organs.reserve(getNumberOfOrgans()); // just for speed up
    for (const auto& o : this->baseOrgans) {
        o->getOrgans(ot, organs);
    }
    return organs;
}

/**
 * Returns a single scalar parameter for each organ as sequential list,
 * corresponding to the sequential organ list, see Organism::getOrgans.
 *
 * This method is mostly for post processing, since it is flexible but slow.
 *
 * @param name      name of the parameter
 * @param ot        the expected organ type, where -1 denotes all organ types (default).
 * @param organs    optionally, a predefined sequential organ list can be used (@param ot is ignored in this case)
 * @return A vector of one parameter values per each organ, if unknown NaN
 */
std::vector<double> Organism::getParameter(std::string name, int ot, std::vector<Organ*> organs) const
{
    if (organs.empty()) {
        organs = getOrgans(ot);
    }
    std::vector<double> p = std::vector<double>(organs.size());
    for (int i=0; i<organs.size(); i++) {
        p[i] = organs[i]->getParameter(name);
    }
    return p;
}

/**
 * Returns the summed parameter, obtained by Organism::getParameters (e.g. getSummed("length"))
 *
 * @param name      name of the parameter
 * @param ot        the expected organ type, where -1 denotes all organ types (default)
 * @return          the summed up value
 */
double Organism::getSummed(std::string name, int ot) const
{
    auto v = getParameter(name, ot);
    return std::accumulate(v.begin(), v.end(), 0.0);
}

/**
 * Returns the organisms number of segments of a specific organ type
 *
 * @param ot        the expected organ type, where -1 denotes all organ types (default)
 * @return          total number of segments in the organism of type ot
 */
int Organism::getNumberOfSegments(int ot) const
{
    int s=0;
    auto organs = getOrgans(ot);
    for (const auto& o : organs) {
        s += o->getNumberOfSegments();
    }
    return s;
}

/**
 * Represents the organ's nodes as polyline
 *
 * @param ot        the expected organ type, where -1 denotes all organ types (default)
 * @return          for each organ a vector of nodes
 */
std::vector<std::vector<Vector3d>> Organism::getPolylines(int ot) const
{
    auto organs = getOrgans(ot);
    std::vector<std::vector<Vector3d>> nodes = std::vector<std::vector<Vector3d>>(organs.size());
    for (size_t j=0; j<organs.size(); j++) {
        std::vector<Vector3d>  n = std::vector<Vector3d>(organs[j]->getNumberOfNodes());
        for (size_t i=0; i<organs[j]->getNumberOfNodes(); i++) { // loop over all nodes of all organs
            n.at(i) = organs[j]->getNode(i);
        }
        nodes[j] = n;
    }
    return nodes;
}

/**
 * The corresponding node creation times to the organ polyline representation (@see Organism::getPolylines)
 *
 * @param ot        the expected organ type, where -1 denotes all organ types (default)
 * @return          for each organ a vector of nodes
 */
std::vector<std::vector<double>> Organism::getPolylinesCTs(int ot) const
{
    auto organs = getOrgans(ot);
    std::vector<std::vector<double>> nodes = std::vector<std::vector<double>>(organs.size());
    for (size_t j=0; j<organs.size(); j++) {
        std::vector<double>  nct = std::vector<double>(organs[j]->getNumberOfNodes());
        for (size_t i=0; i<organs[j]->getNumberOfNodes(); i++) { // loop over all nodes of all organs
            nct.at(i) = organs[j]->getNodeCT(i);
        }
        nodes[j] = nct;
    }
    return nodes;
}

/**
 * All nodes of emerged organs are ordered by their node index,
 * initial nodes of base organs are copied, even if not emerged.
 *
 * @return          a vector of nodes
 */
std::vector<Vector3d> Organism::getNodes() const
{
    auto organs = getOrgans();
    std::vector<Vector3d> nv = std::vector<Vector3d>(getNumberOfNodes()); // reserve big enough vector
    for (const auto& o : baseOrgans) { // copy initial nodes (even if organs have not developed)
        nv.at(o->getNodeId(0)) = o->getNode(0);
    }
    for (const auto& o : organs) { // copy all organ nodes
        for (size_t i=0; i<o->getNumberOfNodes(); i++) {
            nv.at(o->getNodeId(i)) = o->getNode(i);
        }
    }
    return nv;
}

/**
 * The organim's node creation times of a specific organ type corresponding to Organism::getNodes.
 *
 * @return          a vector of node creation times
 */
std::vector<double> Organism::getNodeCTs() const
{
    auto organs = getOrgans();
    std::vector<double> cts = std::vector<double>(getNumberOfNodes()); // reserve big enough vector
    for (const auto& o : baseOrgans) { // copy initial nodes (even if organs have not developed)
        cts.at(o->getNodeId(0)) = o->getNodeCT(0);
    }
    for (const auto& o : organs) { // copy all organ creation times
        for (size_t i=0; i<o->getNumberOfNodes(); i++) {
            cts.at(o->getNodeId(i)) = o->getNodeCT(i);
        }
    }
    return cts;
}

/**
 * The line segments of the organism, each segment consisting of two node indices,
 * corresponding to the node list Organism::getNode(-1),
 * or the node indices from Organism::getNodeId(ot).
 *
 * @param ot        the expected organ type, where -1 denotes all organ types (default)
 * @return          line segments
 */
std::vector<Vector2i> Organism::getSegments(int ot) const
{
    auto organs = getOrgans(ot);
    std::vector<Vector2i> segs = std::vector<Vector2i>(0);
    int nos = 0;
    for (const auto& o : organs) {
        nos += o->getNumberOfSegments();
    }
    segs.reserve(nos); // for speed up
    for (const auto& o : organs) {
        auto s = o->getSegments();
        segs.insert(segs.end(), s.begin(), s.end()); // append s; todo check if it works..
    }
    return segs;
}

/**
 * The creation time per segment, corresponding to Organism::getSegments(ot).
 * The segment creation time equals the node creation time of the second node.
 *
 * @param ot        the expected organ type, where -1 denotes all organ types (default)
 * @return          creation times of each segment
 */
std::vector<double> Organism::getSegmentCTs(int ot) const
{
    auto nodeCT = getNodeCTs(); // of all nodes (otherwise indices are tricky)
    auto segs = getSegments(ot);
    std::vector<double> cts = std::vector<double>(segs.size());
    for (int i=0; i<cts.size(); i++) {
        cts[i] = nodeCT[segs[i].y]; // segment creation time is the node creation time of the second node
    }
    return cts;
}

/**
 * A vector of pointers to the organs containing each segment, corresponding to Organism::getSegments.
 *
 * @param ot        the expected organ type, where -1 denotes all organ types (default)
 * @return          creation times of each segment
 */
std::vector<Organ*> Organism::getSegmentOrigins(int ot) const
{
    auto organs = getOrgans(ot);
    std::vector<Organ*> segs = std::vector<Organ*>(0);
    for (const auto& o : organs) {
        auto s = o->getSegments();
        for (int i=0; i<s.size(); i++) {
            segs.push_back(o);
        }
    }
    return segs;
}

/**
 * @return the indices of the nodes that were moved during the last time step
 */
std::vector<int> Organism::getUpdatedNodeIndices() const
{
    auto organs = this->getOrgans();
    std::vector<int> ni = std::vector<int>(0);
    for (const auto& o : organs) {
        if (o->hasMoved()>0){
            ni.push_back(o->getNodeId(o->getOldNumberOfNodes()-1));
        }
    }
    return ni;
}

/**
 * @return the new coordinates of the nodes that were updated during the last time step
 */
std::vector<Vector3d> Organism::getUpdatedNodes() const
{
    auto organs = this->getOrgans();
    std::vector<Vector3d> nv = std::vector<Vector3d>(0);
    for (const auto& o : organs) {
        if (o->hasMoved()>0){
            nv.push_back(o->getNode(o->getOldNumberOfNodes()-1));
        }
    }
    return nv;
}

/**
 * @return a vector of all nodes created during the last time step
 * to dynamically add to the old node list, see also RootSystem::getNodes()
 */
std::vector<Vector3d> Organism::getNewNodes() const
{
    auto organs = this->getOrgans();
    std::vector<Vector3d> nv(this->getNumberOfNewNodes());
    for (const auto& o : organs) {
        int onon = o->getOldNumberOfNodes();
        for (size_t i=onon; i<o->getNumberOfNodes(); i++) { // loop over all new nodes
            nv.at(o->getNodeId(i)-this->oldNumberOfNodes) = o->getNode(i);
        }
    }
    return nv;
}

/**
 * Creates a vector of new created segments that were created during the last time step
 *
 * @param ot        the expected organ type, where -1 denotes all organ types (default)
 * @return          a vector of newly created segments
 */
std::vector<Vector2i> Organism::getNewSegments(int ot) const
{
    auto organs = this->getOrgans(ot);
    std::vector<Vector2i> si(this->getNumberOfNewNodes());
    int c=0;
    for (const auto& o : organs) {
        int onon = std::abs(o->getOldNumberOfNodes());
        for (size_t i=onon-1; i<o->getNumberOfNodes()-1; i++) {
            Vector2i v(o->getNodeId(i),o->getNodeId(i+1));
            si.at(c) = v;
            c++;
        }
    }
    return si;
}

/**
 * Creates a vector of pointers to the organ class containing the segments,
 * for each newly created segment of organ type @param ot,
 * corresponding to the segments obtained by Organism::getNewSegments(ot)
 *
 * @param ot        the expected organ type, where -1 denotes all organ types (default)
 * @return          a vector of pointers to organs
 */
std::vector<Organ*> Organism::getNewSegmentOrigins(int ot) const
{
    auto organs = this->getOrgans(ot);
    std::vector<Organ*> si(this->getNumberOfNewNodes());
    int c=0;
    for (auto& r:organs) {
        int onon = std::abs(r->getOldNumberOfNodes());
        for (size_t i=onon-1; i<r->getNumberOfNodes()-1; i++) {
            si.at(c) = r;
            c++;
        }
    }
    return si;
}

/**
 * Creates a vector of segment creation times for each newly created segment of organ type @param ot,
 * corresponding to the segments obtained by Organism::getNewSegments(ot)
 *
 * @param ot        the expected organ type, where -1 denotes all organ types (default)
 * @return          a vector of segment creation times
 */
std::vector<double> Organism::getNewSegmentCTs(int ot) const
{
    auto organs = this->getOrgans();
    std::vector<double> nodeCTs(this->getNumberOfNewNodes());
    for (const auto& r : organs) {
        int onon = std::abs(r->getOldNumberOfNodes());
        for (size_t i=onon; i<r->getNumberOfNodes(); i++) { // loop over all new nodes
            nodeCTs.at(r->getNodeId(i)-this->oldNumberOfNodes) = r->getNodeCT(i); // pray that ids are correct

        }
    }
    return nodeCTs;
}

/**
 * @return Quick info about the object for debugging
 */
std::string Organism::toString() const
{
    std::stringstream str;
    str << "Organism with "<< baseOrgans.size() <<" base organs, " << getNumberOfNodes()
                            << " nodes, and a total of " << getNumberOfOrgans() << " organs, after " << getSimTime() << " days";
    return str.str();
}

/**
 * Polymorphic XML parameter file reader:
 * adds all organ parameter types of a XML file to the organism's parameters
 *
 * Sets of organ type parameters are created by OrganTypeParameter::copy.
 * For this each OrganTypeParameter must have already one prototype
 * in the organ type parameters Organism::organParam.
 *
 * @param name      file name
 * @param basetag   name of the base tag (e.g. "organism", or "plant")
 */
void Organism::readParameters(std::string name, std::string basetag)
{
    tinyxml2::XMLDocument doc;
    doc.LoadFile(name.c_str());
    if(doc.ErrorID() == 0) {
        tinyxml2::XMLElement* base = doc.FirstChildElement(basetag.c_str());
        auto p = base->FirstChildElement();
        while(p) {
            std::string tagname = p->Name();
            std::cout << "Organism::readParameter: reading tag "<< tagname << std::endl << std::flush;
            int ot = Organism::organTypeNumber(tagname);
            OrganTypeParameter* otp = organParam[ot].begin()->second->copy(this);
            otp->readXML(p);
            setOrganTypeParameter(otp);
            p = p->NextSiblingElement();
        }
    } else {
        std::cout << "readXML(): could not open file\n" << std::flush;
    }
}

/**
 * XML parameter file writer
 * writes all organ parameter types into a XML File
 *
 * @param name      file name
 * @param basetag   name of the base tag (e.g. "organism", or "plant") *
 * @param comments  write parameter descriptions
 */
void Organism::writeParameters(std::string name, std::string basetag, bool comments) const
{
    tinyxml2::XMLDocument xmlDoc;
    tinyxml2:: XMLElement* xmlParams = xmlDoc.NewElement(basetag.c_str()); // RSML
    for (int ot = 0; ot < numberOfOrganTypes; ot++) {  // delete organ type parameters
        for (auto& otp : organParam[ot]) {
            xmlParams->InsertEndChild(otp.second->writeXML(xmlDoc, comments));
        }
    }
    xmlDoc.InsertEndChild(xmlParams);
    xmlDoc.SaveFile(name.c_str());
}

/**
 * Creates a rsml file with filename @param name.
 *
 * @param name      name of the rsml file
 */
void Organism::writeRSML(std::string name) const
{
    tinyxml2::XMLDocument xmlDoc;
    tinyxml2:: XMLElement* rsml = xmlDoc.NewElement("rsml"); // RSML
    tinyxml2:: XMLElement* meta = getRSMLMetadata(xmlDoc);
    tinyxml2:: XMLElement* scene = getRSMLScene(xmlDoc);
    rsml->InsertEndChild(meta);
    rsml->InsertEndChild(scene);
    xmlDoc.InsertEndChild(rsml);
    xmlDoc.SaveFile(name.c_str());
}

/**
 * @return the meta tag of the rsml file
 */
tinyxml2:: XMLElement* Organism::getRSMLMetadata(tinyxml2::XMLDocument& xmlDoc) const
{
    tinyxml2:: XMLElement* metadata = xmlDoc.NewElement("metadata"); // META
    tinyxml2:: XMLElement* version = xmlDoc.NewElement("version");
    version->SetText(1);
    tinyxml2:: XMLElement* unit = xmlDoc.NewElement("unit");
    unit->SetText("cm");
    tinyxml2:: XMLElement* resolution = xmlDoc.NewElement("resolution");
    resolution->SetText(1);
    tinyxml2:: XMLElement* last = xmlDoc.NewElement("last-modified");
    std::time_t t = std::time(0);
    std::tm* now = std::localtime(&t);
    std::string s = std::to_string(now->tm_mday)+"-"+std::to_string(now->tm_mon+1)+"-"+std::to_string(now->tm_year + 1900);
    last->SetText(s.c_str());
    tinyxml2:: XMLElement* software = xmlDoc.NewElement("software");
    software->SetText("OrganicBox");
    // todo no image tag (?)
    // todo property-definitions
    // todo time sequence (?)
    metadata->InsertEndChild(version);
    metadata->InsertEndChild(unit);
    metadata->InsertEndChild(resolution);
    metadata->InsertEndChild(last);
    metadata->InsertEndChild(software);
    // todo insert remaining tags
    return metadata;
}

/**
 * @return the scene tag of the RSML document, calls base organs to write their tags
 */
tinyxml2:: XMLElement* Organism::getRSMLScene(tinyxml2::XMLDocument& xmlDoc) const
{
    tinyxml2:: XMLElement* scene = xmlDoc.NewElement("scene");
    tinyxml2:: XMLElement* plant = xmlDoc.NewElement("plant");
    scene->InsertEndChild(plant);
    for (auto& o: baseOrgans) {
        o->writeRSML(xmlDoc, plant);
    }
    return scene;
}

/**
 * Sets the seed of the organisms random number generator.
 * In order to obtain two exact same organisms call before Organism::initialize().
 *
 * @param seed      the random number generator seed
 */
void Organism::setSeed(unsigned int seed)
{
    this->gen = std::mt19937(seed);
}


} // namespace
