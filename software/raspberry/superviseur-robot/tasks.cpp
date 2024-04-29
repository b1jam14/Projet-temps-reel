/*
 * Copyright (C) 2018 dimercur
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "tasks.h"
#include <stdexcept>

// Déclaration des priorités des taches
#define PRIORITY_TSERVER 30
#define PRIORITY_TOPENCOMROBOT 20
#define PRIORITY_TMOVE 20
#define PRIORITY_TSENDTOMON 22
#define PRIORITY_TRECEIVEFROMMON 25
#define PRIORITY_TSTARTROBOT 20
#define PRIORITY_TCAMERA 21
#define PRIORITY_TBATTERY 31
#define PRIORITY_TERROR 19
#define PRIORITY_WATCHDOG 19

/*
 * Some remarks:
 * 1- This program is mostly a template. It shows you how to create tasks, semaphore
 *   message queues, mutex ... and how to use them
 * 
 * 2- semDumber is, as name say, useless. Its goal is only to show you how to use semaphore
 * 
 * 3- Data flow is probably not optimal
 * 
 * 4- Take into account that ComRobot::Write will block your task when serial buffer is full,
 *   time for internal buffer to flush
 * 
 * 5- Same behavior existe for ComMonitor::Write !
 * 
 * 6- When you want to write something in terminal, use cout and terminate with endl and flush
 * 
 * 7- Good luck !
 */

/**
 * @brief Initialisation des structures de l'application (tâches, mutex, 
 * semaphore, etc.)
 */
void Tasks::Init() {
    int status;
    int err;

    /**************************************************************************************/
    /* 	Mutex creation                                                                    */
    /**************************************************************************************/
    if (err = rt_mutex_create(&mutex_monitor, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_robot, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_robotStarted, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_move, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_error, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_cam, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_fluxCam, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_watchdog, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    cout << "Mutexes created successfully" << endl << flush;

    /**************************************************************************************/
    /* 	Semaphors creation       							  */
    /**************************************************************************************/
    if (err = rt_sem_create(&sem_barrier, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_openComRobot, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_serverOk, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_startRobot, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_watchdog, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    cout << "Semaphores created successfully" << endl << flush;

    /**************************************************************************************/
    /* Tasks creation                                                                     */
    /**************************************************************************************/
    if (err = rt_task_create(&th_server, "th_server", 0, PRIORITY_TSERVER, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_sendToMon, "th_sendToMon", 0, PRIORITY_TSENDTOMON, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_receiveFromMon, "th_receiveFromMon", 0, PRIORITY_TRECEIVEFROMMON, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_openComRobot, "th_openComRobot", 0, PRIORITY_TOPENCOMROBOT, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_startRobot, "th_startRobot", 0, PRIORITY_TSTARTROBOT, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_move, "th_move", 0, PRIORITY_TMOVE, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_battery, "th_battery", 0, PRIORITY_TBATTERY, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_cam, "th_cam", 0, PRIORITY_TCAMERA, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_watchdog, "th_watchdog", 0, PRIORITY_WATCHDOG, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_error, "th_error", 0, PRIORITY_TERROR, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    cout << "Tasks created successfully" << endl << flush;

    /**************************************************************************************/
    /* Message queues creation                                                            */
    /**************************************************************************************/
    if ((err = rt_queue_create(&q_messageToMon, "q_messageToMon", sizeof (Message*)*50, Q_UNLIMITED, Q_FIFO)) < 0) {
        cerr << "Error msg queue create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    cout << "Queues created successfully" << endl << flush;

    camera = new Camera(sm,10);
    
}







/**
 * @brief Démarrage des tâches
 */
void Tasks::Run() {
    rt_task_set_priority(NULL, T_LOPRIO);
    int err;

    if (err = rt_task_start(&th_server, (void(*)(void*)) & Tasks::ServerTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_sendToMon, (void(*)(void*)) & Tasks::SendToMonTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_receiveFromMon, (void(*)(void*)) & Tasks::ReceiveFromMonTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_openComRobot, (void(*)(void*)) & Tasks::OpenComRobot, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_startRobot, (void(*)(void*)) & Tasks::StartRobotTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_move, (void(*)(void*)) & Tasks::MoveTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_battery, (void(*)(void*)) & Tasks::LevelBat, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_cam, (void(*)(void*)) & Tasks::cam, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_watchdog, (void(*)(void*)) & Tasks::watchdog, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }

    cout << "Tasks launched" << endl << flush;
}

/**
 * @brief Arrêt des tâches
 */
void Tasks::Stop() {
    monitor.Close();
    robot.Close();
    camera -> Close();
}

/**
 */
void Tasks::Join() {
    cout << "Tasks synchronized" << endl << flush;
    rt_sem_broadcast(&sem_barrier);
    pause();
}

/**
 * @brief Thread handling server communication with the monitor.
 */
void Tasks::ServerTask(void *arg) {
    int status;
    
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are started)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /**************************************************************************************/
    /* The task server starts here                                                        */
    /**************************************************************************************/
    rt_mutex_acquire(&mutex_monitor, TM_INFINITE);
    status = monitor.Open(SERVER_PORT);
    rt_mutex_release(&mutex_monitor);

    cout << "Open server on port " << (SERVER_PORT) << " (" << status << ")" << endl;

    if (status < 0) throw std::runtime_error {
        "Unable to start server on port " + std::to_string(SERVER_PORT)
    };
    monitor.AcceptClient(); // Wait the monitor client
    cout << "Rock'n'Roll baby, client accepted!" << endl << flush;
    rt_sem_broadcast(&sem_serverOk);
}

/**
 * @brief Thread sending data to monitor.
 */
void Tasks::SendToMonTask(void* arg) {
    Message *msg;
    
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /**************************************************************************************/
    /* The task sendToMon starts here                                                     */
    /**************************************************************************************/
    rt_sem_p(&sem_serverOk, TM_INFINITE);

    while (1) {
        cout << "wait msg to send" << endl << flush;
        msg = ReadInQueue(&q_messageToMon);
        cout << "Send msg to mon: " << msg->ToString() << endl << flush;
        rt_mutex_acquire(&mutex_monitor, TM_INFINITE);
        monitor.Write(msg); // The message is deleted with the Write
        rt_mutex_release(&mutex_monitor);
    }
}

/**
 * @brief Thread receiving data from monitor.
 */
void Tasks::ReceiveFromMonTask(void *arg) {
    Message *msgRcv;
    
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    /**************************************************************************************/
    /* The task receiveFromMon starts here                                                */
    /**************************************************************************************/
    rt_sem_p(&sem_serverOk, TM_INFINITE);
    cout << "Received message from monitor activated" << endl << flush;

    while (1) {
        msgRcv = monitor.Read();
        cout << "Rcv <= " << msgRcv->ToString() << endl << flush;

        if (msgRcv->CompareID(MESSAGE_MONITOR_LOST)) {
            delete(msgRcv);
            exit(-1);
        } else if (msgRcv->CompareID(MESSAGE_ROBOT_COM_OPEN)) {
            rt_sem_v(&sem_openComRobot);
        } else if (msgRcv->CompareID(MESSAGE_ROBOT_GO_FORWARD) ||
                msgRcv->CompareID(MESSAGE_ROBOT_GO_BACKWARD) ||
                msgRcv->CompareID(MESSAGE_ROBOT_GO_LEFT) ||
                msgRcv->CompareID(MESSAGE_ROBOT_GO_RIGHT) ||
                msgRcv->CompareID(MESSAGE_ROBOT_STOP)) {

            rt_mutex_acquire(&mutex_move, TM_INFINITE);
            move = msgRcv->GetID();
            rt_mutex_release(&mutex_move);
        } else if (msgRcv->CompareID(MESSAGE_CAM_OPEN)){
            openCam();
        } else if (msgRcv->CompareID(MESSAGE_CAM_CLOSE)){
            closeCam();
        } else if (msgRcv->CompareID(MESSAGE_ROBOT_START_WITHOUT_WD)){
            cout << "******** Start without Watchdog ********"<< endl << flush;
            rt_mutex_acquire(&mutex_watchdog, TM_INFINITE);
            wdMode = 0;
            rt_mutex_release(&mutex_watchdog);
            rt_sem_v(&sem_startRobot);
        } else if (msgRcv->CompareID(MESSAGE_ROBOT_START_WITH_WD)) {
            cout << "******** Start with Watchdog ********"<< endl << flush;
            rt_mutex_acquire(&mutex_watchdog, TM_INFINITE);
            wdMode = 1;
            rt_mutex_release(&mutex_watchdog);
            
            rt_sem_v(&sem_startRobot);
            rt_sem_v(&sem_watchdog);
        }
        delete(msgRcv); // mus be deleted manually, no consumer
    }
}

/**
 * @brief Thread opening communication with the robot.
 */
void Tasks::OpenComRobot(void *arg) {
    int status;
    int err;

    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    /**************************************************************************************/
    /* The task openComRobot starts here                                                  */
    /**************************************************************************************/
    while (1) {
        rt_sem_p(&sem_openComRobot, TM_INFINITE);
        cout << "Open serial com (";
        rt_mutex_acquire(&mutex_robot, TM_INFINITE);
        status = robot.Open();
        rt_mutex_release(&mutex_robot);
        cout << status;
        cout << ")" << endl << flush;

        Message * msgSend;
        if (status < 0) {
            msgSend = new Message(MESSAGE_ANSWER_NACK);
        } else {
            msgSend = new Message(MESSAGE_ANSWER_ACK);
        }
        WriteInQueue(&q_messageToMon, msgSend); // msgSend will be deleted by sendToMon
    }
}

/**
 * @brief Thread starting the communication with the robot.
 */
void Tasks::StartRobotTask(void *arg) {
    int watchdog;

    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    /**************************************************************************************/
    /* The task startRobot starts here                                                    */
    /**************************************************************************************/
    while (1) {
        rt_task_wait_period(NULL);

        Message * msgSend;
        rt_sem_p(&sem_startRobot, TM_INFINITE);

        rt_mutex_acquire(&mutex_watchdog, TM_INFINITE);
        watchdog = wdMode;
        rt_mutex_release(&mutex_watchdog);

        if(wdMode == 1){
            cout << "Start robot with watchdog (";
            rt_mutex_acquire(&mutex_robot, TM_INFINITE);
            msgSend = robot.Write(robot.StartWithWD());
            rt_mutex_release(&mutex_robot);
            cout << msgSend->GetID();
            cout << ")" << endl;
            rt_sem_v(&sem_watchdog);
        }else{
            cout << "Start robot without watchdog (";
            rt_mutex_acquire(&mutex_robot, TM_INFINITE);
            msgSend = robot.Write(robot.StartWithoutWD());
            rt_mutex_release(&mutex_robot);
            cout << msgSend->GetID();
            cout << ")" << endl;
        }

        cout << "Movement answer: " << msgSend->ToString() << endl << flush;
        WriteInQueue(&q_messageToMon, msgSend);  // msgSend will be deleted by sendToMon

        if (msgSend->GetID() == MESSAGE_ANSWER_ACK) {
            rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
            robotStarted = 1;
            rt_mutex_release(&mutex_robotStarted);
        }
    }
}

/**
 * @brief Thread handling control of the robot.
 */
void Tasks::MoveTask(void *arg) {
    int rs;
    int cpMove;
    
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    /**************************************************************************************/
    /* The task starts here                                                               */
    /**************************************************************************************/
    rt_task_set_periodic(NULL, TM_NOW, 100000000);

    while (1) {
        rt_task_wait_period(NULL);
        cout << "Periodic movement update";
        rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
        rs = robotStarted;
        rt_mutex_release(&mutex_robotStarted);
        if (rs == 1) {
            rt_mutex_acquire(&mutex_move, TM_INFINITE);
            cpMove = move;
            rt_mutex_release(&mutex_move);
            
            cout << " move: " << cpMove << endl << flush;
            
            rt_mutex_acquire(&mutex_robot, TM_INFINITE);
            robotAnswer = robot.Write(new Message((MessageID)cpMove));
            rt_mutex_release(&mutex_robot);

            if(robotAnswer->CompareID(MESSAGE_ANSWER_ROBOT_TIMEOUT)){
                rt_mutex_acquire(&mutex_error, TM_INFINITE);
                errorCount++;
                cout << " Error: " << errorCount << endl << flush;
                rt_mutex_release(&mutex_error);    
            }else{
                rt_mutex_acquire(&mutex_error, TM_INFINITE);
                errorCount = 0;
                rt_mutex_release(&mutex_error);
            }
        }
        cout << endl << flush;
    }
}

/**
 * Write a message in a given queue
 * @param queue Queue identifier
 * @param msg Message to be stored
 */
void Tasks::WriteInQueue(RT_QUEUE *queue, Message *msg) {
    int err;
    if ((err = rt_queue_write(queue, (const void *) &msg, sizeof ((const void *) &msg), Q_NORMAL)) < 0) {
        cerr << "Write in queue failed: " << strerror(-err) << endl << flush;
        throw std::runtime_error{"Error in write in queue"};
    }
}

/**
 * Read a message from a given queue, block if empty
 * @param queue Queue identifier
 * @return Message read
 */
Message *Tasks::ReadInQueue(RT_QUEUE *queue) {
    int err;
    Message *msg;

    if ((err = rt_queue_read(queue, &msg, sizeof ((void*) &msg), TM_INFINITE)) < 0) {
        cout << "Read in queue failed: " << strerror(-err) << endl << flush;
        throw std::runtime_error{"Error in read in queue"};
    }/** else {
        cout << "@msg :" << msg << endl << flush;
    } /**/

    return msg;
}

// Fonctionnalité 13
void Tasks::LevelBat(){
    /*every 500ms send the battery level*/
    int rs;
    Message* bLevel;
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;

    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);


    /***************/
    /*  Level Bat  */
    /***************/
    rt_task_set_periodic(NULL, TM_NOW, 500000000);

    while(1){
        rt_task_wait_period(NULL);
        cout << "Battery check" << endl << flush;

        rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
        rs = robotStarted;
        // Unlock mutex
        rt_mutex_release(&mutex_robotStarted);
        
         if (rs == 1) { 
            // Get battery from robot 
            rt_mutex_acquire(&mutex_robot, TM_INFINITE);
            bLevel=robot.Write(robot.GetBattery());
            rt_mutex_release(&mutex_robot);
            
            cout << "Battery Level: " << bLevel->ToString() << endl << flush;
            // Display battery level on monitor - write to monitor 
            WriteInQueue(&q_messageToMon, bLevel);  
        }
    }
}

/*
void Tasks::cam(){
    Message * msg;
    int fluxCam;
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    rt_sem_p(&sem_barrier, TM_INFINITE);

    rt_task_set_periodic(NULL, TM_NOW, 100000000);
    while(1){
        rt_task_wait_period(NULL);
        
        rt_mutex_acquire(&mutex_fluxCam, TM_INFINITE);
        fluxCam = flux;
        rt_mutex_release(&mutex_fluxCam);
        if(fluxCam){
            rt_mutex_acquire(&mutex_cam,TM_INFINITE);
            if (camera -> IsOpen()) {
                Img capture = camera -> Grab();
                msg = new MessageImg(MESSAGE_CAM_IMAGE,&capture);
                WriteInQueue(&q_messageToMon, msg);
            }
            rt_mutex_release(&mutex_cam);
        }
    }
}

void Tasks::closeCam(){
    cout << "******** Close Cam ********"<< endl << flush;
    bool status;
    Message * msg;

    rt_mutex_acquire(&mutex_cam, TM_INFINITE);
    camera -> Close();
    rt_mutex_release(&mutex_cam);

    rt_mutex_acquire(&mutex_cam, TM_INFINITE);
    status = camera -> IsOpen();
    rt_mutex_release(&mutex_cam);

    if(status){
        msg = new Message(MESSAGE_ANSWER_ACK);
    }else{
        msg = new Message(MESSAGE_ANSWER_NACK);
    }
    WriteInQueue(&q_messageToMon, msg);

    rt_mutex_acquire(&mutex_fluxCam, TM_INFINITE);
    flux = 0;
    cout << "******** Flux Cam = " << flux << " ********"<< endl << flush;
    rt_mutex_release(&mutex_fluxCam);
}*/

void Tasks::openCam() {
    cout << "******** Opening Cam ********"<< endl << flush;
    Message * msg;
    bool status;

    rt_mutex_acquire(&mutex_cam, TM_INFINITE);

    if(!camera->Open()) {
        msg = new Message(MESSAGE_ANSWER_NACK);
        cout << "cam not opened " << endl;
    }else {
        msg=new Message(MESSAGE_ANSWER_ACK);
        cout << "cam opened" << endl;
        rt_mutex_acquire(&mutex_fluxCam, TM_INFINITE);
        flux = 1;
        rt_mutex_release(&mutex_fluxCam);
    }

    rt_mutex_release(&mutex_cam);
    WriteInQueue(&q_messageToMon,msg); 
    cout << endl << flush; 
}

void Tasks::cam(){
    Message * msg;
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    rt_sem_p(&sem_barrier, TM_INFINITE);
    rt_task_set_periodic(NULL, TM_NOW, 100000000);

    while(1){
        rt_task_wait_period(NULL);
        if (flux){
            rt_mutex_acquire(&mutex_cam, TM_INFINITE);
            Img image = camera->Grab();
            rt_mutex_release(&mutex_cam);

            //rt_mutex_acquire(&mutex_img, TM_INFINITE);
            msg = new MessageImg(MESSAGE_CAM_IMAGE, &image);
            //rt_mutex_release(&mutex_img);
            monitor.Write(msg);

            cout << "Image sent" << endl;
            cout << endl << flush; 
        }else{
            //rt_mutex_release(&mutex_img);
            continue;
        }
    }
}

void Tasks::closeCam() {

    cout << "******** Close Cam ********"<< endl << flush;
    bool status;
    Message * msg;
    /*
    
    rt_mutex_acquire(&mutex_cam, TM_INFINITE);
    status = camera -> IsOpen();
    rt_mutex_release(&mutex_cam);

    rt_mutex_acquire(&mutex_fluxCam, TM_INFINITE);
    flux = 0;
    cout << "******** Flux Cam = " << flux << " ********"<< endl << flush;
    rt_mutex_release(&mutex_fluxCam);

    if(status){
        rt_mutex_acquire(&mutex_cam, TM_INFINITE);
        camera -> Close();
        rt_mutex_release(&mutex_cam);
    }

    if(status){
        msg = new Message(MESSAGE_ANSWER_NACK);
    }else{
        msg = new Message(MESSAGE_ANSWER_ACK);
    }
    */

    rt_mutex_acquire(&mutex_cam, TM_INFINITE);

    rt_mutex_acquire(&mutex_fluxCam, TM_INFINITE);
    flux = 0;
    cout << "******** Flux Cam = " << flux << " ********"<< endl << flush;
    rt_mutex_release(&mutex_fluxCam);

    camera->Close();

    msg = new Message(MESSAGE_ANSWER_ACK);

    rt_mutex_release(&mutex_cam);

    WriteInQueue(&q_messageToMon, msg);

    cout << "camera closed" << endl <<flush;
}

void Tasks::watchdog(){
    int rs;
    int errors;
    Message * msg;

    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    rt_sem_p(&sem_barrier, TM_INFINITE);

    rt_task_set_periodic(NULL, TM_NOW, 100000000);
    while(1){
        rt_task_wait_period(NULL);
        rt_sem_p(&sem_watchdog, TM_INFINITE);

        rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
        int rs = robotStarted;
        rt_mutex_release(&mutex_robotStarted);

        if(rs==1){

            rt_mutex_acquire(&mutex_robot, TM_INFINITE);
            msg = robot.Write(robot.ReloadWD()); 
            
            if(msg->CompareID(MESSAGE_ANSWER_ROBOT_TIMEOUT)){
                rt_mutex_acquire(&mutex_error, TM_INFINITE);
                errorCount++;
                rt_mutex_release(&mutex_error);
            }else{
                rt_mutex_acquire(&mutex_error, TM_INFINITE);
                errorCount=0;
                rt_mutex_release(&mutex_error);
            }

            rt_mutex_acquire(&mutex_error, TM_INFINITE);
            errors=errorCount;
            rt_mutex_release(&mutex_error);

            if(errors==3){
                rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
                robotStarted = 0;
                Stop();
                rt_mutex_release(&mutex_robotStarted);
            }

            rt_mutex_release(&mutex_robot);
            
        }
    }
}
