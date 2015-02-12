#!/usr/bin/python

## system imports/includes
from multiprocessing import Pipe,Process
import subprocess
import os
import yarp

yarp.Network.init()


class worldStateCommunication:
    ## for rpc communication with worldStateManager

    def __init__(self):
        self._rpc_client = yarp.RpcClient()
        self._port_name = "/planner/wsm_rpc:o"
        self._rpc_client.open(self._port_name)
        ## self._rpc_client.addOutput("/wsm/rpc:i") 

    def _execute(self,cmd):
        message = yarp.Bottle()
        message.clear()
        map(message.addString, [cmd])
        ans = yarp.Bottle()
        self._rpc_client.write(message, ans)
        return ans
    def _is_success(self, ans):
        return ans.size() == 1 and ans.get(0).asVocab() == yarp.Vocab.encode("ok")


class ActionExecutorCommunication:
    ## for rpc communication with action executor

    def __init__(self):
        self._rpc_client = yarp.RpcClient()
        self._port_name = "/planner/actInt_rpc:o"
        self._rpc_client.open(self._port_name)
##        self._rpc_client.addOutput("/activityInterface/rpc:i") ## need to verify the port!!!!

    def _execute(self, PathName, cmd):
        Objects_file = open(''.join(PathName +"/Object_names-IDs.dat"))
        Object_list = Objects_file.read().split(';')
        Objects_file.close()
        for k in range(len(Object_list)):
            Object_list[k] = Object_list[k].replace('(','').replace(')','').split(',')
        cmd = cmd.split('_')
        if 'on' in cmd:
            obj = cmd[3]
            hand = cmd[5].replace('()','')
            act = cmd[0]
        else:
            act = cmd[0]
            obj = cmd[1]
            hand = cmd[3].replace('()','')
        for k in range(len(Object_list)):
            if str(act) == Object_list[k][0]:
                act = Object_list[k][1]
            if str(obj) == Object_list[k][0]:
                obj = Object_list[k][1]
            if str(hand) == Object_list[k][0]:
                hand = Object_list[k][1].replace('hand','')
        print act, obj, hand
        message = yarp.Bottle()
        message.clear()
        map(message.addString, [act, obj, hand])
        ans = yarp.Bottle()
        self._rpc_client.write(message, ans)
        return ans
    def _is_success(self, ans):
        return ans.size() == 1 and ans.get(0).asVocab() == 27503
    
def update_state(PathName):
    symbol_file = open(''.join(PathName +"/symbols.dat"))
    symbols = symbol_file.read()
    symbol_file.close()
    data = symbols.split('\n')
    symbols = []
    data.pop()
    for i in range(len(data)):
        aux_data = data[i].split(' ')
        symbols = symbols + [[aux_data[0], aux_data[2]]]
    state_file = open(''.join(PathName +"/state.dat"))
    state = state_file.read()
    state_file.close()
    data = state.replace('-','').replace('()','').replace('\n','')
    data = data.split(' ')
    state = state.replace('\n','').replace(r'(.+?)_touch_(.+?)','')
    for j in range(len(symbols)):
        if symbols[j][0] not in data and symbols[j][1] == 'primitive':
            state = '-'.join((state,''.join((symbols[j][0],'() '))))
    state = ''.join((state,'\n'))
    state_file = open(''.join(PathName +"/state.dat"),'w')
    state_file.write(state)
    state_file.close()
        

def planning_cycle():
    ## mode definition: 1-with praxicon; 2-debug with opc; 3-debug without opc; 4- normal demo 

    mode = 2

    rf = yarp.ResourceFinder()
    rf.setVerbose(True)
    rf.setDefaultContext("poeticon")
    PathName = rf.findPath("contexts/poeticon")
    print(''.join("cd " + PathName +" && " + "./planner.exe"))
    world_rpc = worldStateCommunication()
    motor_rpc = ActionExecutorCommunication()
    
    geo_yarp = yarp.BufferedPortBottle()##
    geo_yarp.open("/planner/grounding_cmd:io")
    
    goal_yarp = yarp.BufferedPortBottle()##
    goal_yarp.open("/planner/goal_cmd:io")
    
    State_yarp = yarp.BufferedPortBottle()##
    State_yarp.open("/planner/opc_cmd:io")

    prax_yarp = yarp.BufferedPortBottle()##
    prax_yarp.open("/planner/prax_inst:io")


    while 1:
        old_state = []
        if mode == 1 or mode == 4:
            while 1:
                prax_bottle_in = prax_yarp.read(False)
                if prax_bottle_in:
                    prax_instr = prax_bottle_in.toString()
                    break
                print 'waiting for praxicon...'
                    
        while 1:
            if State_yarp.getOutputCount() != 0:
                break

    ## cycle that will check if we have any object on the table. if not, it won't continue
        if mode != 3:
            print "opc mode engaged"
            yarp.Time.delay(0.5)
            while 1:
                print "attempting communication"
                if world_rpc._is_success(world_rpc._execute("update")):
                    yarp.Time.delay(0.2)
                    state_flag = 0
                    State_bottle_out = State_yarp.prepare()
                    State_bottle_out.clear()
                    State_bottle_out.addString('update')
                    State_yarp.write()
                
                    while 1:
                        print 'waiting...'
                        State_bottle_in = State_yarp.read(False)
                        if State_bottle_in:
                            print 'Bottle:', State_bottle_in.toString()
                            Object_file = open(''.join(PathName +"/Object_names-IDs.dat"))
                            Objects = Object_file.read().split(';')
                            if len(Objects) > 3:
                                state_flag = 1
                                break
                            else:
                                print 'number of objects too low, updating'
                                break
                        yarp.Time.delay(1)
                    if state_flag == 1:
                        break
                    yarp.Time.delay(1)
                yarp.Time.delay(1)
    ###################
        
        print 'state updated'    
        command = ''
        while 1:
            if goal_yarp.getOutputCount() != 0:
                break
        print 'goal connection done'
        goal_bottle_out = goal_yarp.prepare()
        goal_bottle_out.clear()
        goal_bottle_out.addString('start')
        goal_yarp.write()
        while 1:
            goal_bottle_in = goal_yarp.read(False)
            print 'waiting...'
            if goal_bottle_in:
                command = goal_bottle_in.toString()
                break
            yarp.Time.delay(1)
        print 'goal is done'
        
##        goal_file = open(''.join(PathName +"/final_goal.dat"))
        subgoalsource_file = open(''.join(PathName +"/subgoals.dat"))
##        goal = goal_file.read().split(' ')
##        goal_file.close()
        plan_level = 0

        subgoals = subgoalsource_file.read().split('\n')
        subgoalsource_file.close()
        aux_subgoals = []
        for t in range(len(subgoals)):
            aux_subgoals = aux_subgoals + [subgoals[t].split(' ')]
        print 'started'
        comm = raw_input('update rules? y/n')
        if comm == 'y':
            while 1:
                if geo_yarp.getOutputCount() != 0:
                    break
            geo_bottle_out = geo_yarp.prepare()
            geo_bottle_out.clear()
            geo_bottle_out.addString('update')
            geo_yarp.write()
            while 1:
                print 'waiting....'
                geo_bottle_in = geo_yarp.read(False)
                if geo_bottle_in:
                    command = geo_bottle_in.toString()
                if command == 'ready':
                    print 'ready'
                    break
                yarp.Time.delay(1)
        rules_file = open(''.join(PathName +"/rules.dat"),'r')
        old_rules = rules_file.read().split('\n')
        rules_file.close()
        update_state(PathName)

        config_file = open(''.join(PathName +"/config"),'r')
        config_data = config_file.read().split('\n')
        for w in range(len(config_data)):
            if config_data[w].find('[PRADA]') != -1:
                horizon = 5
                config_data[w+2] = 'PRADA_horizon %d' %horizon
                break
        config_file.close()
        config_file = open(''.join(PathName +"/config"), 'w')
        for w in range(len(config_data)):
            config_file.write(config_data[w])
            config_file.write('\n')
        config_file.close()
        
        while(True):
            ## Plan!!!
            flag_kill = 0
            cont = 0
            if mode != 3:
                while 1:
##                    choice_var = raw_input("update state? y/n")
##                    if choice_var == 'n':
##                        break
                    print "communicating..."
                    state_flag = 0
                    State_bottle_out = State_yarp.prepare()
                    State_bottle_out.clear()
                    State_bottle_out.addString('update')
                    State_yarp.write()
                
                    while 1:
                        print 'waiting...'
                        State_bottle_in = State_yarp.read(False)
                        if State_bottle_in:
                            Object_file = open(''.join(PathName +"/Object_names-IDs.dat"))
                            Objects = Object_file.read().split(';')
                            if len(Objects) > 3:
                                state_flag = 1
                                break
                            else:
                                print 'number of objects too low, updating'
                                break
                        yarp.Time.delay(1)
                    if state_flag == 1:
                        break
                    yarp.Time.delay(1)
                   
            update_state(PathName)
            
################# function under construction, updating when objects change ################
## requires the geometric grounding to change, to ground object by object
## will take two types of commands: a "complete", to make the full grounding
##                                  an object ID, to update only for that object
##
##
## if len(Objects) > len(old_Objects):
##    print 'new object found, updating...'
##    new_obj = []
##    while h in range(len(Objects)):
##        if Objects[h] not in old_Objects:
##            new_obj = new_obj + [Objects[h].split(',')[0].replace('(','')]
##    geo_bottle_out = geo_yarp.prepare()
##    geo_bottle_out.clear()
##    geo_bottle_out.addString('update ' + new_obj)
##    geo_yarp.write()
##    while 1:
##        print 'waiting....'
##        yarp.Time.delay(1)
##        geo_bottle_in = geo_yarp.read(False)
##        if geo_bottle_in:
##             command = geo_bottle_in.toString()
##             if command == 'ready':
##                 print 'ready'
##                 break
##    old_Objects = Objects
                    
            state_file = open(''.join(PathName +"/state.dat"),'r')
            state = state_file.read().split(' ')
            state[-1] = state[-1].replace('\r','').replace('\n','')
            state_file.close()
            not_to_add = []

            #####################################
            if state == old_state:
                print "state hasn't changed, action failed"
                for t in range(len(rules)):
                    if rules[t].replace(' ','').replace('\n','').replace('\r','') == next_action and next_action != '':
                        print rules[t]
                        print rules[t+4]
                        p = 0
                        while 1:
                            if rules[t+p] == '':
                                print rules[t]
                                adapt_rules = rules[t+4].split(' ')
                                print adapt_rules
                                adapt_rules[2] = str(float(adapt_rules[2])/2)
                                rules[t+4] = ' '.join(adapt_rules)
                                adapt_noise = rules[t+p-1].split(' ')
                                adapt_noise[2] = str(float(adapt_noise[2])+float(adapt_rules[2]))
                                print rules[t+p-1]
                                rules[t+p-1] = ' '.join(adapt_noise)
                                print rules[t+p-1]
                                break
                            p = p+1
                        rules_file = open(''.join(PathName +"/rules.dat"),'w')
                        print ''.join(PathName + "/rules.dat")
                        for y in range(len(rules)):
                            rules_file.write(rules[y])
                            rules_file.write('\n')
                        rules_file.close()
                        print "rules adapted"
                        break

            subgoal_file = open(''.join(PathName +"/goal.dat"),'w')
            subgoal_file.write(subgoals[plan_level])
            subgoal_file.close()
            if plan_level >= len(subgoals)-1:
                print 'plan finished'
##                geo_bottle_out = geo_yarp.prepare()
##                geo_bottle_out.clear()
##                geo_bottle_out.addString('kill')
##                geo_yarp.write()
##                goal_bottle_out = goal_yarp.prepare()
##                goal_bottle_out.clear()
##                goal_bottle_out.addString('kill')
##                goal_yarp.write()
                break
            print("process-planner.exe")
            print(''.join(PathName + "/planner.exe"))
            planner = subprocess.Popen([''.join(PathName + "/planner.exe")],stdout = subprocess.PIPE, stderr = subprocess.PIPE,cwd = PathName)
            data = planner.communicate()
            
            data = data[0].split('\n')
            rules_file = open(''.join(PathName +"/rules.dat"),'r')
            rules = rules_file.read().split('\n')
            rules_file.close()
            next_action = []
            for t in range(len(data)):
                if data[t] == 'The planner would like to kindly recommend the following action to you:':
                    next_action = data[t+1]
            if next_action == []:
                next_action = data[-2].split(' ')[0]
                if '  %s' %next_action not in rules:
                    config_file = open(''.join(PathName +"/config"),'r')
                    config_data = config_file.read().split('\n')
                    for w in range(len(config_data)):
                        if config_data[w].find('[PRADA]') != -1:
                            temp_var = config_data[w+2].split(' ')
                            horizon = int(temp_var[1])
                            horizon = horizon + 1
                            config_data[w+2] = 'PRADA_horizon %d' %horizon
                            break
                    config_file.close()
                    config_file = open(''.join(PathName +"/config"), 'w')
                    for w in range(len(config_data)):
                        config_file.write(config_data[w])
                        config_file.write('\n')
                    config_file.close()
                    
            print '\nprocessing...\n'
            
            ## processes output of planner
            ## executes next action
            
            subgoal_file = open(''.join(PathName +"/goal.dat"),'r')
            goal = subgoal_file.read().split(' ')
            subgoal_file.close()
            print 'goals not met:'
            for t in range(len(goal)):
                if goal[t] not in state:
                    print goal[t]
                    cont = 1
            print '\n'
            
            holding_symbols = []
            for t in range(len(aux_subgoals[plan_level-1])):
                if aux_subgoals[plan_level-1][t] in aux_subgoals[plan_level]:
                    holding_symbols = holding_symbols + [aux_subgoals[plan_level-1][t]]
            if holding_symbols != []:
                holding_symbols.pop(-1)
            if plan_level >= 1:
                for t in range(len(holding_symbols)):
                    if holding_symbols[t] not in state:
                        print holding_symbols[t]
                        failed_steps = plan_level
                        cont = -1
                        print 'situation changed, receding in plan'
                        break
            print 'continue:',cont
            if cont == -1:
                plan_level = plan_level-1
                config_file = open(''.join(PathName +"/config"),'r')
                config_data = config_file.read().split('\n')
                for w in range(len(config_data)):
                    if config_data[w].find('[PRADA]') != -1:
                        horizon = 5
                        config_data[w+2] = 'PRADA_horizon %d' %horizon
                        break
                config_file.close()
                config_file = open(''.join(PathName +"/config"), 'w')
                for w in range(len(config_data)):
                    config_file.write(config_data[w])
                    config_file.write('\n')
                config_file.close()
            if cont == 0:
                rules_file = open(''.join(PathName +"/rules.dat"),'w')
                for y in range(len(old_rules)):
                    rules_file.write(old_rules[y])
                    rules_file.write('\n')
                rules_file.close()
                plan_level = plan_level+1
                config_file = open(''.join(PathName +"/config"),'r')
                config_data = config_file.read().split('\n')
                for w in range(len(config_data)):
                    if config_data[w].find('[PRADA]') != -1:
                        horizon = 5
                        config_data[w+2] = 'PRADA_horizon %d' %horizon
                        break
                config_file.close()
                config_file = open(''.join(PathName +"/config"), 'w')
                for w in range(len(config_data)):
                    config_file.write(config_data[w])
                    config_file.write('\n')
                config_file.close()    
                rules_file = open(''.join(PathName +"/rules.dat"),'r')
                rules = rules_file.read().split('\n')
                rules_file.close()
            act_check = '  %s' %next_action
            if act_check in rules:
                print 'action to be executed: ', next_action, '\n'
                motor_rpc._is_success(motor_rpc._execute(PathName, next_action))
                world_rpc._is_success(world_rpc._execute("update"))
                raw_input("press any key")



                old_state = state
                ## send action to motor executor
##                await for reply
##                if reply == 'fail':

                            

    #########################################################################
    ##			Communication with action executor
    ##            ARE_yarp_bottle_out = ARE_yarp.prepare()
    ##            ARE_yarp_bottle_out.clear()
    ##            ARE_yarp_bottle_out.addString(next_action)
    ##            ARE_yarp.write()
    ##            ARE_command = ''
    ##            while 1:
    ##                ARE_yarp_bottle_in = ARE_yarp.read(False)
    ##                if ARE_yarp_bottle_in:
    ##                    ARE_command = ARE_yarp_bottle_in.toString()
    ##                if ARE_command == '[ack]':
    ##                    print 'action confirmed'
    ##                    break
    ##                if ARE_command == '[nack]':
    ##                    print 'action not acknowledged, stopping'
    ##                    flag_kill = 1
    ##                    break
    ##########################################################################
                
                if flag_kill == 1:
                    break

    ##########################################################################
    ##			DEBUG MODE ONLY					##
                if mode == 3:
                    next_state = []
                    for t in range(len(rules)):
                        if rules[t] == '  %s' %next_action:
                            next_state = rules[t+4].split(' ')
                            next_state.pop(2)
                            next_state.pop(1)
                            next_state.pop(0)
                            break
                    for t in range(len(next_state)):
                        if next_state[t].find('-') == 0:
                            selec_state = next_state[t]
                            for b in range(len(state)):
                                if selec_state == state[b]:
                                    not_to_add = not_to_add + [selec_state]
                            selec_state = selec_state.replace('-','',1)
                            for b in range(len(state)):
                                if selec_state == state[b]:
                                    not_to_add = not_to_add + [selec_state]
                        if next_state[t].find('-') == -1:
                            selec_state = next_state[t]
                            for b in range(len(state)):
                                if selec_state == state[b]:
                                    not_to_add = not_to_add + [selec_state]
                            selec_state = '-%s' %next_state[t]
                            for b in range(len(state)):
                                if selec_state == state[b]:
                                    not_to_add = not_to_add + [selec_state]
                    temp_state = []
                    
                    for t in range(len(state)):
                        if state[t] not in not_to_add:
                            temp_state = temp_state + [state[t]]
                    state = temp_state + next_state
                    state = ' '.join(state)
                    state_file = open("state.dat",'w')
                    state_file.write(state)
                    state_file.close()
    ##########################################################################
                
            print 'planning step: ' ,plan_level
            print 'planning horizon: ',horizon
            if horizon > 15:
                print 'cant find a solution, abandoning plan'
##                geo_bottle_out = geo_yarp.prepare()
##                geo_bottle_out.clear()
##                geo_bottle_out.addString('kill')
##                geo_yarp.write()
##                goal_bottle_out = goal_yarp.prepare()
##                goal_bottle_out.clear()
##                goal_bottle_out.addString('kill')
##                goal_yarp.write()
                break
    if flag_kill == 1:
        geo_bottle_out = geo_yarp.prepare()
        geo_bottle_out.clear()
        geo_bottle_out.addString('kill')
        geo_yarp.write()
        goal_bottle_out = goal_yarp.prepare()
        goal_bottle_out.clear()
        goal_bottle_out.addString('kill')
        goal_yarp.write()

if __name__ == '__main__':
    planning_cycle()    