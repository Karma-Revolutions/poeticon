#include "affordanceCommunicationModule.h"

bool affComm::configure(ResourceFinder &rf)
{
    // module parameters
    vector<string> temp_vect;
    moduleName = rf.check("name", Value("affordanceCommunication")).asString();
    PathName = rf.findPath("contexts/"+rf.getContext());
    setName(moduleName.c_str());

    closing = false;

    temp_vect.clear();
    temp_vect.push_back("1");
    temp_vect.push_back("grasp");
    translation.push_back(temp_vect);
    temp_vect.clear();
    temp_vect.push_back("2");
    temp_vect.push_back("drop");
    translation.push_back(temp_vect);
    temp_vect.clear();
    temp_vect.push_back("3");
    temp_vect.push_back("put");
    translation.push_back(temp_vect);
    temp_vect.clear();
    temp_vect.push_back("4");
    temp_vect.push_back("pull");
    translation.push_back(temp_vect);
    temp_vect.clear();
    temp_vect.push_back("5");
    temp_vect.push_back("push");
    translation.push_back(temp_vect);
    temp_vect.clear();
    temp_vect.push_back("6");
    temp_vect.push_back("reach");
    translation.push_back(temp_vect);
    // translation = {{"1","grasp"},{"2","drop"},{"3","put"},{"4","pull"},{"5","push"},{"6","reach"}};

    if (PathName==""){
        cout << "path to contexts/"+rf.getContext() << " not found" << endl;
        return false;    
    }
    else {
        cout << "Context FOUND!" << endl;
    }
    return true;
}

bool affComm::updateModule()
{
    return !closing;
}

void affComm::openPorts()
{
    geoPort.open("/affordanceCommunication/ground_cmd:io");
    plannerPort.open("/affordanceCommunication/planner_cmd:io");
    affnetPort.open("/affordanceCommunication/aff_query:io");

    // RPC ports
    descQueryPort.open("/affordanceCommunication/opc2prada_query:io");
    actionQueryPort.open("/affordanceCommunication/actInt_rpc:o");
}

bool affComm::close()
{
    cout << "closing..." << endl;
    closing = true;
    plannerPort.close();
    geoPort.close();
    affnetPort.close();

    descQueryPort.close();
    actionQueryPort.close();
    return true;
}

bool affComm::interrupt()
{
    cout << "interrupting ports" << endl;
    plannerPort.interrupt();
    geoPort.interrupt();
    affnetPort.interrupt();

    descQueryPort.interrupt();
    actionQueryPort.interrupt();
    return true;
}

bool affComm::plannerCommand()
{
    if (plannerPort.getInputCount() == 0)
    {
        cout << "planner not connected" << endl;
        return false;
    }
    while (true){
        plannerBottle = plannerPort.read(false);
        if (plannerBottle != NULL){
            command = plannerBottle->toString().c_str();
            return true;
        }
    }
}

bool affComm::loadObjs()
{
    string objFileName, line, temp_str;
    ifstream objFile;
    vector<string> aux_objects, temp_vect;
    objFileName = PathName + "/Object_names-IDs.dat";
    objFile.open(objFileName.c_str());
    if (!objFile.is_open())
    {
        cout << "failed to open object file" << endl;
        return false;
    }
    getline(objFile,line);
    aux_objects = split(line, ';');
    for (int i=0; i < aux_objects.size(); ++i)
    {
        temp_str = aux_objects[i];
        temp_str.replace(temp_str.find("("), 1,"");
        temp_str.replace(temp_str.find(")"), 1,"");
        temp_vect = split(temp_str,',');
        objects.push_back(temp_vect);
        if (temp_vect[1] == "stick" || temp_vect[1] == "rake")
        {
            tools.push_back(temp_vect);
        }
    }
    return true;    
}

bool affComm::affordancesCycle()
{
    while (true)
    {
        if (plannerPort.getInputCount() == 0)
        {
            cout << "planner not connected" << endl;
            return false;
        }

        if (plannerCommand())
        {
            if (command == "update")
            {
                Bottle& planner_bottle_out = plannerPort.prepare();
                planner_bottle_out.clear();
                planner_bottle_out.addString("ready");
                plannerPort.write();
            }
        }
        if (command == "query")
        {
            if (!plannerQuery())
            {
                cout << "failed to perform query" << endl;
                return false;
            }
        }
        if (command == "update")
        {
            if (!loadObjs())
            {
                cout << "failed to initialize objects" << endl;
                return false;
            }
            posits.clear();
            if (!updateAffordances())
            {
                cout << "failed to update affordances" << endl;
                return false;
            }
        }

    }
    return true;
}

bool affComm::queryDescriptors()
{
    vector<double> data;
    if (descQueryPort.getInputCount() == 0)
    {
        cout << "opc2prada not connected" << endl;
        return false;
    }
    for (int i = 0; i < objects.size(); ++i)
    {
        if (objects[i][0] != "11" && objects[i][0] != "12")
        {
            cmd.clear();
            cmd.addString("query2d");
            cmd.addInt(atoi(objects[i][0].c_str()));
            descQueryPort.write(cmd,reply);
            if (reply.size() == 1){
                if (reply.toString() != "ACK" && reply.toString() != "()" && reply.toString() != "" && reply.toString() != "[fail]")
                {
                    data.clear();
                    data.push_back(atof(objects[i][0].c_str()));
                    for (int j = 0; j < reply.get(0).asList()->size(); ++j)
                    {
                        data.push_back(reply.get(0).asList()->get(j).asDouble());
                    }
                    descriptors.push_back(data);
                }
                else
                {
                    data.clear();
                    data.push_back(atof(objects[i][0].c_str()));
                    data.push_back(0.0);
                    data.push_back(0.0);
                    data.push_back(0.0);
                    data.push_back(0.0);
                    data.push_back(0.0);
                    data.push_back(0.0);
                    data.push_back(0.0);
                    descriptors.push_back(data);
                }
            }
        }
    }
    return true;
}

bool affComm::queryToolDescriptors()
{
    vector<double> data, temp_vect;
    vector<vector<double> > tool_data;
    for (int i = 0; i < tools.size(); ++i)
    {
        cmd.clear();
        cmd.addString("query2d");
        cmd.addInt(atoi(objects[i][0].c_str()));
        descQueryPort.write(cmd,reply);
        if (reply.size() == 1){
            if (reply.toString() != "ACK" && reply.toString() != "()" && reply.toString() != "" && reply.toString() != "[fail]")
            {
                data.clear();
                tool_data.clear();
                data.push_back(atof(objects[i][0].c_str()));
                tool_data.push_back(data);
                data.clear();
                for (int j = 2; j < reply.get(0).asList()->get(0).asList()->size(); ++j)
                {
                    data.push_back(reply.get(0).asList()->get(0).asList()->get(j).asDouble());
                }
                tool_data.push_back(data);
                data.clear();
                for (int j = 2; j < reply.get(0).asList()->get(1).asList()->size(); ++j)
                {
                    data.push_back(reply.get(0).asList()->get(1).asList()->get(j).asDouble());
                }
                tool_data.push_back(data);
                tooldescriptors.push_back(tool_data);
            }
            else
            {
                tool_data.clear();
                temp_vect.push_back(atof(tools[i][0].c_str()));
                tool_data.push_back(temp_vect);
                temp_vect.clear();
                temp_vect.push_back(0.0);
                temp_vect.push_back(0.0);
                temp_vect.push_back(0.0);
                temp_vect.push_back(0.0);
                temp_vect.push_back(0.0);
                tool_data.push_back(temp_vect);
                temp_vect.clear();
                temp_vect.push_back(0.0);
                temp_vect.push_back(0.0);
                temp_vect.push_back(0.0);
                temp_vect.push_back(0.0);
                temp_vect.push_back(0.0);
                tool_data.push_back(temp_vect);
                tooldescriptors.push_back(tool_data);
            }
        }
    }
    return true;
}

bool affComm::plannerQuery()
{
    if (plannerPort.getInputCount() == 0)
    {
        cout << "planner not connected" << endl;
        return false;
    }
    vector<double> new_posits;
    vector<string> toolsIDs;
    string message = "", temp_str;
    for (int i = 0; i < posits.size(); ++i)
    {
        std::ostringstream sin;
        sin << posits[i][0];
        temp_str = sin.str();
        if (find_element(toolsIDs, temp_str) == 0)
        {
            toolsIDs.push_back(temp_str);
            new_posits.push_back(posits[i][0]);
            new_posits.push_back(posits[i][1]);
            new_posits.push_back(posits[i][2]);
            
            std::ostringstream sin1, sin2, sin3;
            sin1 << int(posits[i][0]);
            sin2 << int(posits[i][1]);
            sin3 << int(posits[i][2]);
            message = message + sin1.str() + " " + sin2.str() + " " + sin3.str() + " ";
        }
    }
    Bottle& planner_bottle_out = plannerPort.prepare();
    planner_bottle_out.clear();
    planner_bottle_out.addString(message);
    plannerPort.write();
    return true;
}

bool affComm::updateAffordances()
{
    string comm;
    vector<string> data, temp_vect;
    while (true)
    {
        if (geoPort.getInputCount() == 0)
        {
            cout << "geometric grounding module not connected." << endl;
            return false;
        }
        Affor_bottle_in = geoPort.read(false);
        if (Affor_bottle_in)
        {
            comm = Affor_bottle_in->toString();
            break;
        }
    }
    if (comm == "done")
    {
        command = "";
    }
    if (comm == "update")
    {
        while (true)
        {
            Affor_bottle_in = geoPort.read(false);
            if (Affor_bottle_in)
            {
                data.clear();
                for (int i = 0; i < Affor_bottle_in->get(0).asList()->size(); ++i)
                {
                    data.push_back(Affor_bottle_in->get(0).asList()->get(i).asString());
                }
                break;
            }
        }
        rule = data[0];
        context = data[1];
        outcome = data[2];
        outcome2 = data[3];
        outcome3 = data[4];
        for (int i = 0; i < translation.size(); ++i)
        {
            act.clear();
            act = split(rule, '_');
            while (true)
            {
                if (act[0].find(" ") != std::string::npos)
                {
                    act[0].replace(act[0].find(" "),1,"");
                }
                else if (act[3].find("(") != std::string::npos)
                {
                    act[3].replace(act[3].find("("),2,"");
                }
                else
                {
                    break;
                }
            }
            if (translation[i][1] == act[0])
            {
                if (translation[i][1] == "grasp")
                {
                    if (!getGraspAff())
                    {
                        cout << "failed to obtain grasping affordances" << endl;
                        return false;
                    }
                }
                if (translation[i][1] == "push")
                {
                    if (!getPushAff())
                    {
                        cout << "failed to obtain pushing affordances" << endl;
                        return false;
                    }
                }
                if (translation[i][1] == "pull")
                {
                    if (!getPullAff())
                    {
                        cout << "failed to obtain pulling affordances" << endl;
                        return false;
                    }
                }
                if (translation[i][1] == "drop")
                {
                    if (!getDropAff())
                    {
                        cout << "failed to obtain dropping affordances" << endl;
                        return false;
                    }
                }
                if (translation[i][1] == "put")
                {
                    if (!getPutAff())
                    {
                        cout << "failed to obtain putting affordances" << endl;
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

bool affComm::sendOutcomes()
{
    if (geoPort.getInputCount() == 0)
    {
        cout << "geometric grounding module not connected" << endl;
        return false;
    }
    Bottle& Affor_bottle_out = geoPort.prepare();
    Affor_bottle_out.clear();
    Affor_bottle_out.addString(outcome);
    Affor_bottle_out.addString(outcome2);
    Affor_bottle_out.addString(outcome3);
    geoPort.write();
    return true;
}

bool affComm::getGraspAff()
{
    cout << "Grasp affordances not implemented yet, going for default" << endl;
    if (!sendOutcomes())
    {
        cout << "failed to send probabilities to the grounding module" << endl;
        return false;
    }
    return true;
}

bool affComm::getDropAff()
{
    cout << "Drop affordances not implemented yet, going for default" << endl;
    if (!sendOutcomes())
    {
        cout << "failed to send probabilities to the grounding module" << endl;
        return false;
    }
    return true;
}

bool affComm::getPutAff()
{
    cout << "Put affordances not implemented yet, going for default" << endl;
    if (!sendOutcomes())
    {
        cout << "failed to send probabilities to the grounding module" << endl;
        return false;
    }
    return true;
}

bool affComm::getPushAff()
{
    vector<double> data, obj_desc, tool_desc1, tool_desc2;
    vector<string> new_outcome, new_outcome2;
    string obj, tool, new_outcome_string, new_outcome2_string;
    double prob_succ1, prob_succ2, prob_fail, prob_succ;
    int toolnum=0;
    if (affnetPort.getInputCount() == 0)
    {
        cout << "affordance network not connected, going for default" << endl;
        if (!sendOutcomes())
        {
            cout << "failed to send probabilities to the grounding module" << endl;
            return false;
        }
        if (act[1] != "11" && act[1] != "12" && act[3] != "11" && act[3] != "12")
        {
            for (int j = 0; j < tooldescriptors.size(); ++j)
            {
                data.clear();
                data.push_back(strtof(act[3].c_str(), NULL));
                data.push_back(tooldescriptors[j][1][0]);
                data.push_back(tooldescriptors[j][1][1]);
                posits.push_back(data);
            }
        }
        return true;
    }
    else
    {
        if (act[1] != "11" && act[1] != "12" && act[3] != "11" && act[3] != "12")
        {
            Bottle& affnet_bottle_out = affnetPort.prepare();
            affnet_bottle_out.clear();
            obj = act[1];
            tool = act[3];
            for (int o = 0; o < descriptors.size(); ++o)
            {
                if (descriptors[o][0] == strtof(obj.c_str(), NULL))
                {
                    obj_desc = descriptors[o];
                    obj_desc.erase(obj_desc.begin());
                }
            }
            for (int o = 0; o < tooldescriptors.size(); ++o)
            {
                if (tooldescriptors[o][0][0] == strtof(tool.c_str(),NULL))
                {
                    tool_desc1 = tooldescriptors[o][1];
                    tool_desc2 = tooldescriptors[o][2];
                    toolnum = o;
                }
            }
            if (tooldescriptors[toolnum][1][1] > tooldescriptors[toolnum][2][1])
            {
                for (int j = 3; j < tool_desc1.size(); ++j)
                {
                    affnet_bottle_out.addDouble(tool_desc1[j]);
                }
                for (int j = 1; j < obj_desc.size(); ++j)
                {
                    affnet_bottle_out.addDouble(obj_desc[j]);
                }
                affnet_bottle_out.addDouble(2.0);
                affnetPort.write();
                while (true)
                {
                    affnet_bottle_in = affnetPort.read(false);
                    if (affnet_bottle_in)
                    {
                        if (affnet_bottle_in->toString() == "[nack]")
                        {
                            cout << "query failed, using default" << endl;
                            if (!sendOutcomes())
                            {
                                cout << "failed to send probabilities to grounding module" << endl;
                                return false;
                            } 
                            return true;
                        }
                        break;
                    }
                }
                prob_succ1 = 0.0;
                for (int g = 0; g < affnet_bottle_in->get(0).asList()->size(); ++g)
                {
                    if (g < 2)
                    {
                        for (int j = 0; j < affnet_bottle_in->get(0).asList()->get(g).asList()->size(); ++j)
                        {
                            prob_succ1 = prob_succ1 + affnet_bottle_in->get(0).asList()->get(g).asList()->get(g).asDouble();
                        }
                    }
                }
                if (prob_succ1 >= 0.95)
                {
                    prob_succ1 = 0.95;
                }
                if (prob_succ1 < 0.5)
                {
                    prob_succ1 = 0.5;
                }
                prob_succ = prob_succ1;
                data.clear();
                data.push_back(strtof(tool.c_str(), NULL));
                data.push_back(tooldescriptors[toolnum][1][0]);
                data.push_back(tooldescriptors[toolnum][1][1]);
                posits.push_back(data);
            }
            else
            {
                for (int j = 3; j < tool_desc1.size(); ++j)
                {
                    affnet_bottle_out.addDouble(tool_desc1[j]);
                }
                for (int j = 1; j < obj_desc.size(); ++j)
                {
                    affnet_bottle_out.addDouble(obj_desc[j]);
                }
                affnet_bottle_out.addDouble(2.0);
                affnetPort.write();
                while (true)
                {
                    affnet_bottle_in = affnetPort.read(false);
                    if (affnet_bottle_in)
                    {
                        if (affnet_bottle_in->toString() == "[nack]")
                        {
                            cout << "query failed, using default" << endl;
                            if (!sendOutcomes())
                            {
                                cout << "failed to send probabilities to grounding module" << endl;
                                return false;
                            } 
                            return true;
                        }
                        break;
                    }
                }
                prob_succ2 = 0.0;
                for (int g = 0; g < affnet_bottle_in->get(0).asList()->size(); ++g)
                {
                    if (g < 2)
                    {
                        for (int j = 0; j < affnet_bottle_in->get(0).asList()->get(g).asList()->size(); ++j)
                        {
                            prob_succ1 = prob_succ1 + affnet_bottle_in->get(0).asList()->get(g).asList()->get(g).asDouble();
                        }
                    }
                }
                if (prob_succ2 >= 0.95)
                {
                    prob_succ2 = 0.95;
                }
                if (prob_succ2 < 0.5)
                {
                    prob_succ2 = 0.5;
                }
                prob_succ = prob_succ2;
                data.clear();
                data.push_back(strtof(tool.c_str(), NULL));
                data.push_back(tooldescriptors[toolnum][2][0]);
                data.push_back(tooldescriptors[toolnum][2][1]);
                posits.push_back(data);
            }
            std::ostringstream sin;;
            prob_fail = 1.0 - prob_succ;
            new_outcome = split(outcome, ' ');
            sin << prob_succ;
            new_outcome[2] = sin.str();
            new_outcome2 = split(outcome2, ' ');
            sin << prob_fail;
            new_outcome2[2] = sin.str();
            new_outcome_string = "";
            for (int i = 0; i < new_outcome.size(); ++i)
            {
                new_outcome_string = new_outcome_string + new_outcome[i] + " ";
            }
            outcome = new_outcome_string;
            new_outcome2_string = "";
            for (int i = 0; i < new_outcome2.size(); ++i)
            {
                new_outcome2_string = new_outcome2_string + new_outcome2[i] + " ";
            }
            outcome2 = new_outcome2_string;
            if (!sendOutcomes())
            {
                cout << "failed to send probabilities to grounding module" << endl;
                return false;
            }
        }
        else
        {
            if (!sendOutcomes())
            {
                cout << "failed to send probabilities to grounding module" << endl;
                return false;
            }
        }
        return true;

    }
}
bool affComm::getPullAff()
{
    vector<double> data, obj_desc, tool_desc1, tool_desc2;
    vector<string> new_outcome, new_outcome2;
    string obj, tool, new_outcome_string, new_outcome2_string;
    double prob_succ1, prob_succ2, prob_fail, prob_succ;
    int toolnum=0;
    if (affnetPort.getInputCount() == 0)
    {
        cout << "affordance network not connected, going for default" << endl;
        if (!sendOutcomes())
        {
            cout << "failed to send probabilities to the grounding module" << endl;
            return false;
        }
        if (act[1] != "11" && act[1] != "12" && act[3] != "11" && act[3] != "12")
        {
            for (int j = 0; j < tooldescriptors.size(); ++j)
            {
                data.clear();
                data.push_back(strtof(act[3].c_str(), NULL));
                data.push_back(tooldescriptors[j][2][0]);
                data.push_back(tooldescriptors[j][2][1]);
                posits.push_back(data);
            }
        }
        return true;
    }
    else
    {
        if (act[1] != "11" && act[1] != "12" && act[3] != "11" && act[3] != "12")
        {
            Bottle& affnet_bottle_out = affnetPort.prepare();
            affnet_bottle_out.clear();
            obj = act[1];
            tool = act[3];
            for (int o = 0; o < descriptors.size(); ++o)
            {
                if (descriptors[o][0] == strtof(obj.c_str(), NULL))
                {
                    obj_desc = descriptors[o];
                    obj_desc.erase(obj_desc.begin());
                }
            }
            for (int o = 0; o < tooldescriptors.size(); ++o)
            {
                if (tooldescriptors[o][0][0] == strtof(tool.c_str(),NULL))
                {
                    tool_desc1 = tooldescriptors[o][1];
                    tool_desc2 = tooldescriptors[o][2];
                    toolnum = o;
                }
            }
            if (tooldescriptors[toolnum][1][1] > tooldescriptors[toolnum][2][1])
            {
                for (int j = 3; j < tool_desc1.size(); ++j)
                {
                    affnet_bottle_out.addDouble(tool_desc1[j]);
                }
                for (int j = 1; j < obj_desc.size(); ++j)
                {
                    affnet_bottle_out.addDouble(obj_desc[j]);
                }
                affnet_bottle_out.addDouble(1.0);
                affnetPort.write();
                while (true)
                {
                    affnet_bottle_in = affnetPort.read(false);
                    if (affnet_bottle_in)
                    {
                        if (affnet_bottle_in->toString() == "[nack]")
                        {
                            cout << "query failed, using default" << endl;
                            if (!sendOutcomes())
                            {
                                cout << "failed to send probabilities to grounding module" << endl;
                                return false;
                            } 
                            return true;
                        }
                        break;
                    }
                }
                prob_succ1 = 0.0;
                for (int g = 0; g < affnet_bottle_in->get(0).asList()->size(); ++g)
                {
                    if (g > 2)
                    {
                        for (int j = 0; j < affnet_bottle_in->get(0).asList()->get(g).asList()->size(); ++j)
                        {
                            prob_succ1 = prob_succ1 + affnet_bottle_in->get(0).asList()->get(g).asList()->get(g).asDouble();
                        }
                    }
                }
                if (prob_succ1 >= 0.95)
                {
                    prob_succ1 = 0.95;
                }
                if (prob_succ1 < 0.5)
                {
                    prob_succ1 = 0.5;
                }
                prob_succ = prob_succ1;
                data.clear();
                data.push_back(strtof(tool.c_str(), NULL));
                data.push_back(tooldescriptors[toolnum][1][0]);
                data.push_back(tooldescriptors[toolnum][1][1]);
                posits.push_back(data);
            }
            else
            {
                for (int j = 3; j < tool_desc1.size(); ++j)
                {
                    affnet_bottle_out.addDouble(tool_desc1[j]);
                }
                for (int j = 1; j < obj_desc.size(); ++j)
                {
                    affnet_bottle_out.addDouble(obj_desc[j]);
                }
                affnet_bottle_out.addDouble(1.0);
                affnetPort.write();
                while (true)
                {
                    affnet_bottle_in = affnetPort.read(false);
                    if (affnet_bottle_in)
                    {
                        if (affnet_bottle_in->toString() == "[nack]")
                        {
                            cout << "query failed, using default" << endl;
                            if (!sendOutcomes())
                            {
                                cout << "failed to send probabilities to grounding module" << endl;
                                return false;
                            } 
                            return true;
                        }
                        break;
                    }
                }
                prob_succ2 = 0.0;
                for (int g = 0; g < affnet_bottle_in->get(0).asList()->size(); ++g)
                {
                    if (g > 2)
                    {
                        for (int j = 0; j < affnet_bottle_in->get(0).asList()->get(g).asList()->size(); ++j)
                        {
                            prob_succ1 = prob_succ1 + affnet_bottle_in->get(0).asList()->get(g).asList()->get(g).asDouble();
                        }
                    }
                }
                if (prob_succ2 >= 0.95)
                {
                    prob_succ2 = 0.95;
                }
                if (prob_succ2 < 0.5)
                {
                    prob_succ2 = 0.5;
                }
                prob_succ = prob_succ2;
                data.clear();
                data.push_back(strtof(tool.c_str(), NULL));
                data.push_back(tooldescriptors[toolnum][2][0]);
                data.push_back(tooldescriptors[toolnum][2][1]);
                posits.push_back(data);
            }
            std::ostringstream sin;
            prob_fail = 1.0 - prob_succ;
            new_outcome = split(outcome, ' ');
            sin << prob_succ;
            new_outcome[2] = sin.str();
            new_outcome2 = split(outcome2, ' ');
            sin << prob_fail;
            new_outcome2[2] = sin.str();
            new_outcome_string = "";
            for (int i = 0; i < new_outcome.size(); ++i)
            {
                new_outcome_string = new_outcome_string + new_outcome[i] + " ";
            }
            outcome = new_outcome_string;
            new_outcome2_string = "";
            for (int i = 0; i < new_outcome2.size(); ++i)
            {
                new_outcome2_string = new_outcome2_string + new_outcome2[i] + " ";
            }
            outcome2 = new_outcome2_string;
            if (!sendOutcomes())
            {
                cout << "failed to send probabilities to grounding module" << endl;
                return false;
            }
        }
        else
        {
            if (!sendOutcomes())
            {
                cout << "failed to send probabilities to grounding module" << endl;
                return false;
            }
        }
        return true;

    }
}
