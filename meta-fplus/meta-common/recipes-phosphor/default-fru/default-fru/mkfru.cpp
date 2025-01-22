/*
// Copyright (c) 2019 Intel Corporation
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
// Abstract:   default FRU generation
//
*/

#include <fstream>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <vector>
#include <string.h>
#include "rapidxml.hpp"

using namespace rapidxml;
using namespace std;

constexpr uint8_t fillChar = '.';
constexpr uint8_t eof = 0xc1;
const string vendor = "Yandex LLC";

string mac0 = "UNKNOWN";

xml_node<>* xml_fru = NULL;

// round up to nearest block size (power of 2)
constexpr size_t blockRound(size_t len, size_t blk)
{
    return ((len) + (((blk) - ((len) & ((blk)-1))) & ((blk)-1)));
}

uint8_t mklen(uint8_t len)
{
    return static_cast<uint8_t>((0x3 << 6) | len);
}

struct FruEntry
{
    static constexpr size_t fruBlockSize = 8; // type, length, checksum
    static constexpr size_t fixedBytes = 3;   // type, length, checksum
    FruEntry() = delete;
    FruEntry(const vector<uint8_t>& contents)
    {
        constexpr size_t verOffset = 0;
        constexpr size_t lenOffset = 1;
        value.resize(blockRound(fixedBytes + contents.size(), fruBlockSize));
        value[verOffset] = 1;
        value[lenOffset] = blocks();
        copy(contents.begin(), contents.end(), value.begin() + 2);
        addChecksum();
    }

    void addChecksum()
    {
        int sum = accumulate(value.begin(), value.end(), 0);
        value.back() = static_cast<uint8_t>(256 - sum & 0xff);
    }

    uint8_t blocks() const
    {
        return static_cast<uint8_t>(value.size() / 8);
    }

    vector<uint8_t> value;
};

size_t fillDots(vector<uint8_t>::iterator start, size_t count)
{
    *start++ = mklen(count); // prefix with (0xc0 | count)
    auto end = start + count++;
    fill(start, end, 'X');
    return count;
}

string getXmlFruParam(string paramName)
{
    string rc = "UNKNOWN";
    if (0 == strncmp(paramName.c_str(), "MAC", sizeof("MAC"))) {
        return string("MAC=" + mac0);
    }
     if (! xml_fru)
        return rc;

    xml_node<>* param = xml_fru->first_node(paramName.c_str());
    if (! param)
        return rc;

    return param->value();
}

size_t fillStr(vector<uint8_t>::iterator start, const string& ParamName)
{
   string str = getXmlFruParam(ParamName);
    size_t count = str.size();
    *start++ = mklen(count++); // prefix with (0xc0 | count)
    copy(str.begin(), str.end(), start);
    return count;
}

vector<uint8_t> genChassisContents()
{
    constexpr size_t pnSize = 18;
    constexpr size_t snSize = 18;
    constexpr size_t amSize = 31;
    constexpr size_t headerSize = 1;
    constexpr size_t contentSize = headerSize + 1 + pnSize + 1 + snSize + 1 +
                                   amSize + 1 + amSize + sizeof(eof);
    vector<uint8_t> data(contentSize);
    size_t offset = 0;
    // chassis type (main server chassis)
    data[offset++] = 0x17;
    // chassis part number
    offset += fillStr(data.begin() + offset, "ChassisPartNumber");
    // chassis serial number
    offset += fillStr(data.begin() + offset, "ChassisSerialNumber");
    // info am1
    offset += fillStr(data.begin() + offset, "MAC");
    // info am2
    offset += fillStr(data.begin() + offset, "infoam2");
    data[offset] = eof;

    return data;
}

vector<uint8_t> genBoardContents(const string& name)
{
    constexpr size_t headerSize = 4;
    constexpr size_t snSize = 12;
    constexpr size_t pnSize = 10;
    const string version = "FRU Ver 0.01";
    size_t contentSize = headerSize + 1 + name.size() + 1 + vendor.size() + 1 +
                         snSize + 1 + pnSize + 1 + version.size() + sizeof(eof);
    vector<uint8_t> data(contentSize);
    size_t offset = 0;
    // chassis type (main server chassis)
    data[offset++] = 0; // language code
    data[offset++] = 0; // mfg date/time
    data[offset++] = 0; // mfg date/time
    data[offset++] = 0; // mfg date/time
    // manufacturer name
    offset += fillStr(data.begin() + offset, "BoardManufacturerName");
    // product name
    offset += fillStr(data.begin() + offset, "BoardProductName");
    // board sn
    offset += fillStr(data.begin() + offset, "BoardSerialNumber");
    // board pn
    offset += fillStr(data.begin() + offset, "BoardPartNumber");
    // fru version string
    offset += fillStr(data.begin() + offset, "ProductVersion");
    data[offset] = eof;

    return data;
}

vector<uint8_t> genProductContents(const string& name)
{
    constexpr size_t headerSize = 1;
    constexpr size_t pnSize = 10;
    constexpr size_t pvSize = 20;
    constexpr size_t snSize = 12;
    constexpr size_t atSize = 20;
    constexpr size_t idSize = 0;
    const string version = "FRU Ver 0.01";
    size_t contentSize = headerSize + 1 + vendor.size() + 1 + name.size() + 1 +
                         pnSize + 1 + pvSize + 1 + snSize + 1 + atSize + 1 +
                         idSize + sizeof(eof);
    vector<uint8_t> data(contentSize);
    size_t offset = 0;
    // chassis type (main server chassis)
    data[offset++] = 0; // language code
    // manufacturer name
    offset += fillStr(data.begin() + offset, "ProductManufacturerName");
    // product name
    offset += fillStr(data.begin() + offset, "ProductName");
    // product part number
    offset += fillStr(data.begin() + offset, "ProductPartNumber");
    // product version
    offset += fillStr(data.begin() + offset, "ProductVersion");
    // product serial number
    offset += fillStr(data.begin() + offset, "ProductSerialNumber");
    // product asset tag
    offset += fillStr(data.begin() + offset, "AssetTag");
    // empty fru file id
    offset += fillStr(data.begin() + offset, "fru_file_id");
    data[offset] = eof;

    return data;
}

int createFru(const string& name, const string& filename)
{
    vector<uint8_t> internal{1, 0, 0, 0, 0, 0, 0, 1}; // fixed data
    FruEntry chassis(genChassisContents());
    FruEntry board(genBoardContents(name));
    FruEntry product(genProductContents(name));
    uint8_t offset = 1; // room for header's offset
    FruEntry header({
        offset += 1, // internal size
        offset += chassis.blocks(),
        offset += board.blocks(),
    });
    ofstream output(filename);
    ostream_iterator<uint8_t> outputIter(output);
    copy(header.value.begin(), header.value.end(), outputIter);
    copy(internal.begin(), internal.end(), outputIter);
    copy(chassis.value.begin(), chassis.value.end(), outputIter);
    copy(board.value.begin(), board.value.end(), outputIter);
    copy(product.value.begin(), product.value.end(), outputIter);
    constexpr size_t minFruSize = 0x1ff;
    size_t fruSize = header.value.size() + internal.size() +
                     chassis.value.size() + board.value.size() +
                     product.value.size();
    if (fruSize < minFruSize)
    {
        vector<uint8_t> padding(minFruSize - fruSize);
        copy(padding.begin(), padding.end(), outputIter);
    }
    output.close();
    return 0;
}

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <'xml file'> ['output file']\n";
        return 1;
    }

    xml_document<> doc;
    xml_node<> * root_node;

    ifstream xmlFile(argv[1]);
    if (! xmlFile) {
        std::cerr << "Unable to open file: " << argv[1] << "\n";
        return 1;
    }

    vector<char> buffer((istreambuf_iterator<char>(xmlFile)), istreambuf_iterator<char>());
    buffer.push_back('\0');
    doc.parse<0>(&buffer[0]);
    root_node = doc.first_node();

    /* Find FRU section */
    for(xml_node<>* branch = root_node->first_node(); branch; branch = branch->next_sibling()) {
        if (0 == strncmp(branch->name(), "FRU", sizeof("FRU"))) {
            std::cout << "FRU found\n";
            xml_fru = branch;
            break;
        }
    }

    /* Find NIC MAC address */
    xml_node<>* binfo = root_node->first_node("BoardInfo");
    if (binfo) {
        binfo = binfo->first_node("Main");
        if (binfo) {
            binfo = binfo->first_node("NIC");
            if (binfo) {
                binfo = binfo->first_node("Interface1");
                if (binfo) {
                    binfo = binfo->first_node("MacAddr0");
                    if (binfo) {
                        mac0 =  string(binfo->value());
                        std::cout << "Found NIC MAC addrsess:" << mac0 << "\n";
                    }
                }
            }
        }
    }

    std::cout << "Motherboard name:" << getXmlFruParam("BoardProductName") << "\n";

    string outputName = getXmlFruParam("BoardProductName") + ".fru.bin";
    if (argc == 3) {
        outputName = string(argv[2]);
    }

    return createFru(getXmlFruParam("BoardProductName"), outputName);
}
