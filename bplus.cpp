/*
 * Copyright (c) 2015 Srijan R Shetty <srijan.shetty+code@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#define PREFIX "leaves/leaf_"

#include <iostream>
#include <math.h>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <queue>
#include <vector>
#include <climits>
#include <algorithm>

using namespace std;

class Node {
    // Static data
    static int leafCount;

    // Type of leaf
    bool leaf;

    // LeafNode properties
    string leafFileName;

    public:

    // Bounds
    static int lowerBound;
    static int upperBound;

    // Keys and their associated children
    vector<double> keys;
    vector<Node *> children;

    // The parent of the Node
    Node *parent;

    // Basic initialization
    Node();

    // Check if leaf
    bool isLeaf() { return leaf; }

    // set to internalNode
    void setToInternalNode() { leaf = false; }

    // Return the size of keys
    int size() { return keys.size(); }

    // Initialize the for the tree
    static void initialize(int pageSize);

    // Return the position of a key in keys
    int getKeyPosition(double key);

    // Read all the keys from disk to memory
    void getKeysFromDisk();

    // Insert object into disk
    void insertObject(double key);

    // Insert an internal node into the tree
    void insertNode(double key, Node *newChild);

    // Split the current Leaf Node
    void splitLeaf();

    // Split the current internal Node
    void splitInternal();
};

// Initialize static variables
int Node::lowerBound = 0;
int Node::upperBound = 0;
int Node::leafCount = 0;

// The root node
Node *bRoot = nullptr;

Node::Node() {
    // Initially the parent is NULL
    parent = nullptr;

    // Initially every node is a leaf
    leaf = true;

    // Exit if the lowerBoundKey is not defined
    if (lowerBound == 0) {
        cout << "LowerKeyBound not defined";
        exit(1);
    }

    // LeafNode properties
    leafFileName = PREFIX + to_string(leafCount++);
}

void Node::initialize(int pageSize) {
    int nodeSize = sizeof(new Node());
    int keySize = sizeof(double);
    lowerBound = floor((pageSize - nodeSize) / (2 * (keySize + nodeSize)));
    upperBound = 2 * lowerBound;
}

int Node::getKeyPosition(double key) {
    if (keys.size() == 0) {
        return 0;
    }

    // Find the original position of the key
    int position = -1;
    if (key < keys.front()) {
        position = 0;
    } else {
        for (int i = 1; i < (int)keys.size(); ++i) {
            if (keys[i -1] < key && key <= keys[i]) {
                position = i;
            }
        }

        if (position == -1) {
            position = keys.size();
        }
    }

    return position;
}

void Node::getKeysFromDisk() {
    // Create a binary file
    ifstream leafFile;
    leafFile.open(leafFileName, ios::binary|ios::in);

    // Read the key and enter it into keys
    double key;
    while (!leafFile.eof()) {
        leafFile.read((char *) &key, sizeof(double));

        /* Common Error in reading files */
        if (!leafFile) break;

        keys.push_back(key);
    }

    // Close the file
    leafFile.close();
}

void Node::insertObject(double key) {
    // Add the key to keys
    int position = getKeyPosition(key);
    keys.insert(keys.begin() + position, key);

    // Clean the file
    ofstream leafFile;
    leafFile.open(leafFileName, ios::binary|ios::out);

    // Now rewrite the contents back to file
    for (auto key : keys) {
        leafFile.write((char *) &key, sizeof(key));
    }

    // Close the file
    leafFile.close();
}

void Node::insertNode(double key, Node *newChild) {
    int position = getKeyPosition(key);

    // insert the new key to keys
    keys.insert(keys.begin() + position, key);

    // insert the newChild
    children.insert(children.begin() + position + 1, newChild);

    // If this overflows, we move again upward
    if ((int)keys.size() > upperBound) {
        // splitInternal();
    }
}

void Node::splitInternal() {
    // Create a surrogate internal node
    Node surrogateInternalNode = Node();
    surrogateInternalNode.setToInternalNode();

    // Fix up the keys
    for (auto key = keys.begin() + lowerBound; key != keys.end(); ++key) {
        surrogateInternalNode.keys.push_back(*key);
    }
    keys.resize(lowerBound);
}

void Node::splitLeaf() {
    // Create a surrogate leaf node
    Node *surrogateLeafNode = new Node();
    for (auto key = keys.begin() + lowerBound; key != keys.end(); ++key) {
        surrogateLeafNode->insertObject(*key);
    }

    // Resize the current leaf node
    keys.resize(lowerBound);

    if (parent != nullptr) {
        // Assign parents
        surrogateLeafNode->parent = parent;

        // Now we push up the splitting one level
        parent->insertNode(surrogateLeafNode->keys.front(), surrogateLeafNode);
    } else {
        // Create a new parent node
        Node *newParent = new Node();
        newParent->setToInternalNode();

        // Assign parents
        surrogateLeafNode->parent = newParent;
        parent = newParent;

        // Insert the key into the keys
        newParent->keys.push_back(surrogateLeafNode->keys.front());

        // Insert the children
        newParent->children.push_back(this);
        newParent->children.push_back(surrogateLeafNode);

        // Reset the root node
        // bRoot = newParent;
    }
}

void initialize(int pageSize) {
    // Compute parameters
    Node::initialize(pageSize);

    // Initialize the root
    bRoot = new Node();
}

void insert(Node root, double key) {
    // If the root is a leaf, we can directly insert
    if (root.isLeaf()) {
        root.getKeysFromDisk();

        if (root.size() >= root.upperBound) {
            root.splitLeaf();
        } else {
            root.insertObject(key);
        }
    } else {
        // We traverse the tree
        int position = root.getKeyPosition(key);

        // Recurse into the tree
        insert(*root.children[position + 1], key);
    }
}
