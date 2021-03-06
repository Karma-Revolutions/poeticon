#!/usr/bin/python

# Copyright: (C) 2012-2015 POETICON++, European Commission FP7 project ICT-288382
# CopyPolicy: Released under the terms of the GNU GPL v2.0.
# Copyright: (C) 2015 VisLab, Institute for Systems and Robotics,
#                Instituto Superior Tecnico, Universidade de Lisboa, Lisbon, Portugal
# Author: Alexandre Antunes
# CopyPolicy: Released under the terms of the GNU GPL v2.0

# -*- coding: cp1252 -*-

##                                   Goal Imaginer.py                               ##
######################################################################################
##                      Bottles to be received!!                                    ##
##                                                                                  ##
##      1) World+Robot State module                                                 ##
##          - Sends Query for objects                                               ##
##          - Returns:                                                              ##
##              - Object ID                                                         ##
##              - Object Names                                                      ##
##          - Sends Query for hand clearance                                        ##
##          - Returns:                                                              ##
##              - Hands available, and clear.                                       ##
##                                                                                  ##
##                                                                                  ##
##      - Will create a discrete state                                              ##
##      - Will build the tree using the actions given by the praxicon               ##
##                                                                                  ##
##      IDs:                                                                        ##
##          11 - left hand                                                          ##
##          12 - right hand                                                         ##
######################################################################################
import re
from copy import deepcopy
import yarp

def goal_imaginer():
    yarp.Network.init()
    goal_yarp = yarp.BufferedPortBottle()
    goal_yarp.open("/goal_imag/planner_cmd:io") ##
    rf = yarp.ResourceFinder()
    rf.setVerbose(True)
    rf.setDefaultContext("poeticon")
    PathName = rf.findPath("contexts/poeticon")

    prax_yarp_in = yarp.BufferedPortBottle()##
    prax_yarp_in.open("/goal_imag/prax_inst:i")
    while 1:
        while 1:
            goal_bottle_in = goal_yarp.read(False)
            
            if goal_bottle_in:
                command = goal_bottle_in.toString()
                print command
                break
            yarp.Time.delay(0.1)
        if command == 'praxicon':
            while 1:
                prax_bottle_in = prax_yarp_in.read(False)
                if prax_bottle_in:
                    instructions = []
                    print 'bottle received: \n', prax_bottle_in.toString()
                    if prax_bottle_in.toString().find('a') != -1:
                        for g in range(prax_bottle_in.size()):
                            temp_instructions = []
                            for y in range(prax_bottle_in.get(g).asList().size()):
                                temp1_instructions = []
                                for t in range(prax_bottle_in.get(g).asList().get(y).asList().size()):
                                    temp1_instructions = temp1_instructions + [prax_bottle_in.get(g).asList().get(y).asList().get(t).toString()]
                                    print temp1_instructions
                                temp_instructions = temp_instructions + [temp1_instructions]
                            instructions = instructions + temp_instructions
                        yarp.Time.delay(0.1)
                        goal_bottle_out = goal_yarp.prepare()
                        goal_bottle_out.clear()
                        goal_bottle_out.addString('done')
                        goal_yarp.write()
                        break
            
        if command == 'update':
            print 'starting'
            print instructions
            goal_file = open(''.join(PathName +"/goal.dat"),'w')
            sub_goal_file = open(''.join(PathName +"/subgoals.dat"),'w')
            actions_file = open(''.join(PathName +"/pre_rules.dat"))

            ## this file will be replaced by a call to World+Robot State module
            Object_file = open(''.join(PathName +"/Object_names-IDs.dat"))
            Objects = Object_file.read()
            translat = []
            object_list = re.findall(r'\d+',Objects) ## list of objects IDs
            Objects = Objects.split(';')
            Objects.pop()
            for j in range(len(Objects)):
                translat = translat + [Objects[j].replace('(','').replace(')','').split(',')] ## list of objects IDs+numbers
            actions = actions_file.read().split('\n')

            subgoals = [[]]

            ## instructions = bottle from yarp, ja defino isso
            ## Assumo que as instrucoes vem na forma: ((objecto, accao, objecto),(objecto, accao, objecto))
            ## ou seja, uma lista de listas, cada uma destas a ser uma accao
            ## 
            ## instructions = '((hand,grasp,cheese),(cheese,reach,bun-bottom),(hand,put,cheese),(hand,grasp,salami),(salami,reach,cheese),(hand,put,salami),(hand,grasp,bun-top),(bun-top,reach,salami),(hand,put,bun-top))'
            ## instructions = '((hand,grasp,cheese),(cheese,reach,bun-bottom),(hand,put,cheese),(hand,grasp,bun-top),(bun-top,reach,cheese),(hand,put,bun-top))'
            for g in range(len(instructions)):
                for y in range(len(instructions[g])):
                    instructions[g][y] = instructions[g][y].replace('hand','left')
            ## state = verificar maos (query hand clearance), verificar objectos (query world state)
            ## objects = object name list
            ## we can create a state from here (replacing _obj with the name)

## test function to check if the praxicon and the object_names-IDs have the same objects!!!
##            goal_fail = 0
##            for g in range(len(instructions)):
##                for y in range(len(instructions[g])):
##                    flag_ok = 1
##                    for t in range(len(translat)):
##                        if translat[t][1] == instructions[g][y]:
##                            flag_ok = 0
##                            break
##                    if flag_ok == 1:
##                        goal_fail = 1
##                        break
##                if goal_fail == 1:
##                    print 'unknown objects detected, stopping compiling'
##                    break
##            if goal_fail != 1:
            first_inst = instructions[0]
            for i in range(0,len(actions)):
                if actions[i].find("%s_" %first_inst[1]) != -1:
                    aux_subgoal = actions[i+2].replace('_obj','%s' %first_inst[2]).replace('_tool','%s' %first_inst[0]).replace('_hand','left').split(' ')
                    aux_subgoal.pop(1)
                    aux_subgoal.pop(0)
                    subgoals = [aux_subgoal]
                    break
            aux_subgoal = []
            for i in range(len(instructions)):
                prax_action = instructions[i]
                if prax_action[1] != 'reach':
                    for j in range(0,len(actions)):
                        if actions[j].find("%s_" %prax_action[1]) != -1:
                            if actions[j+4].find('_ALL') != -1:
                                
                                tool = prax_action[0] 
                                obj = prax_action[2] 
                                    
                                new_action = deepcopy(actions)
                                aux_subgoal = actions[j+4].split(' ')
                                new_temp_rule = ['']
                                for u in range(len(aux_subgoal)):
                                    if aux_subgoal[u].find('_ALL') != -1:
                                        temp_rule = new_action[j+4].replace('_obj','%s' %obj).replace('_tool','%s' %tool)
                                        if actions[j].find('_hand'):
                                            temp_rule = temp_rule.replace('_hand','left')
                                        temp_rule = temp_rule.split(' ')
                                        for k in range(len(translat)):
                                            if aux_subgoal[u].replace('_obj','%s' %obj).replace('_tool','%s' %tool).find(translat[k][1]) == -1:
                                                temp_rule = temp_rule+[(aux_subgoal[u].replace('_obj','%s' %obj).replace('_tool','%s' %tool).replace('_ALL', translat[k][1]))]
                                        for w in range(len(temp_rule)):
                                            flag_not_add = 0
                                            if temp_rule[w].find('-') != 0:
                                                var_find = temp_rule[w]
                                            if temp_rule[w].find('-') == 0:
                                                var_find = temp_rule[w].replace('-','',1)
                                            for v in range(len(new_temp_rule)):
                                                if new_temp_rule[v].find(var_find) != -1:
                                                    flag_not_add = 1
                                                    break
                                            if flag_not_add != 1:
                                                new_temp_rule = new_temp_rule + [temp_rule[w]]
                                        new_action[j] = ' '.join(new_temp_rule)
                                                
                                for h in range(len(new_temp_rule)-1,-1,-1):
                                    if new_temp_rule[h].find('_ALL') != -1:
                                        new_temp_rule.pop(h)
                                new_temp_rule.pop(1)
                                new_temp_rule.pop(0)
                                aux_subgoal = new_temp_rule
                                subgoals = subgoals + [subgoals[-1] + aux_subgoal]
                            elif actions[j].find('put_') != -1:
                                tool = prax_action[2] 
                                tool2 = prax_action[0]
                                aux_subgoal = actions[j+4].replace('_obj','%s' %obj).replace('_tool','%s' %tool)
                                if actions[j].find('_hand'):
                                    aux_subgoal = aux_subgoal.replace('_hand','left')
                                aux_subgoal = aux_subgoal.split(' ')
                                aux_subgoal.pop(2)
                                aux_subgoal.pop(1)
                                aux_subgoal.pop(0) ## para retirar a probabilidade
                                subgoals = subgoals + [subgoals[-1]+aux_subgoal]
                            elif actions[j].find('_obj') and actions[j].find('_tool'):
                                tool = prax_action[0] 
                                obj = prax_action[2] 
                                aux_subgoal = actions[j+4].replace('_obj','%s' %obj).replace('_tool','%s' %tool)
                                if actions[j].find('_hand'):
                                    aux_subgoal = aux_subgoal.replace('_hand','left')
                                aux_subgoal = aux_subgoal.split(' ')
                                aux_subgoal.pop(2)
                                aux_subgoal.pop(1)
                                aux_subgoal.pop(0) ## para retirar a probabilidade
                                subgoals = subgoals + [subgoals[-1]+aux_subgoal]
                                    
                            index_var = []
                            for g in range(len(aux_subgoal)):
                                flag_detect = 0
                                if aux_subgoal[g].find('-') == 0:
                                    temp_var = aux_subgoal[g].replace('-','',1)
                                    for h in range(len(subgoals[-1])):
                                        if subgoals[-1][h] == temp_var:
                                            index_var = index_var + [h]
                                        if subgoals[-1][h] == aux_subgoal[g] and flag_detect == 1:
                                            index_var = index_var + [h]
                                        if subgoals[-1][h] == aux_subgoal[g] and flag_detect == 0:
                                            flag_detect = 1
                                else: 
                                    for h in range(len(subgoals[-1])):
                                        if subgoals[-1][h] == ''.join(['-']+[aux_subgoal[g]]):
                                            index_var = index_var + [h]
                                        if subgoals[-1][h] == aux_subgoal[g] and flag_detect == 1:
                                            index_var = index_var + [h]
                                        if subgoals[-1][h] == aux_subgoal[g] and flag_detect == 0:
                                            flag_detect = 1
                            temp_goal  = []
                            for y in range(len(subgoals[-1])):
                                if y not in index_var:
                                    temp_goal = temp_goal + [subgoals[-1][y]]
                            subgoals[-1] = temp_goal
                else:
                    obj = prax_action[2]
            for j in range(len(translat)):
                for h in range(len(subgoals)):
                    for l in range(len(subgoals[h])):
                        subgoals[h][l] = subgoals[h][l].replace('%s' %translat[j][1],'%s' %translat[j][0])
            for i in range(len(subgoals)):
                for j in range(len(subgoals[i])):
                    sub_goal_file.write(subgoals[i][j])
                    if j != len(subgoals[i])-1:
                        sub_goal_file.write(' ')
                sub_goal_file.write('\n')
            sub_goal_file.close()

            for i in range(len(subgoals[-1])):
                goal_file.write(subgoals[-1][i])
                goal_file.write(' ')
            yarp.Time.delay(0.5)
            goal_file.close()
            goal_bottle_out = goal_yarp.prepare()
            goal_bottle_out.clear()
            goal_bottle_out.addString('done')
            goal_yarp.write(True)
        if command == 'kill':
            break
##
##    for y in range(len(subgoals)):
##        print subgoals[y], '\n\n'

goal_imaginer()
    
