//
//  main.cpp
//  Draw My Aurora
//
//  Created by Pujun Lun on 4/6/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#include <iostream>

#include "drawpath.hpp"

using namespace std;

int main(int argc, const char * argv[]) {
    try {
        DrawPath pathEditor;
        pathEditor.mainLoop();
        glfwTerminate();
        return 0;
    } catch (exception& e) {
        cerr << e.what() << endl;
    }
    glfwTerminate();
    return -1;
}
