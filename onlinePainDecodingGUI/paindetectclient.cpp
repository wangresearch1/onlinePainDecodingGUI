#include "paindetectclient.h"
#include "ui_paindetectclient.h"
#include <iostream>
#include <numeric>
#include <iomanip>
#include <cmath>

//consts: max plots per figure==6
// plot color list
const std::vector<QColor> plot_colors = {Qt::red,Qt::green,Qt::blue,Qt::yellow,Qt::magenta,Qt::cyan,Qt::black};
// shaded area color list
const std::vector<QColor> plot_colors_alpha = {QColor(255, 0, 0, 30),QColor(0, 255, 0, 30),QColor(0, 0, 255, 30),QColor(255, 255, 0, 30),QColor(255, 0, 255, 30),QColor(0, 255, 255, 30)};
// pen styles
const std::vector<Qt::PenStyle> plot_penstyle = {Qt::SolidLine,Qt::DashLine,Qt::DotLine,Qt::DashDotLine,Qt::DashDotDotLine};

PainDetectClient::PainDetectClient(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::PainDetectClient),
    collector(nullptr),
    pDo(nullptr),
    rasterRect(nullptr),
    raster(nullptr),
    rasterTextLabels(),
    rasterLine(nullptr),
    cbarLine(nullptr),
    rasterColorScale(nullptr),
    Cbar(nullptr),
    cbarData(),
    cbarx(),
    zplotTextLabels(),
    zplotThreshLines(),
    detection_lines(),
    stim_event_lines(),
    collectTimer(nullptr),
    refreshTimer(nullptr),
    plotTime(8),// in sec
    n_conf(0),
    updateMode(manual),
    optoTriggerMode(manual),
    random_opto_freq(60),
    testMode(Continuous),
    trialStarted(false),
    ctl_trialStarted(false),
    detection_cnt_ready(false),
    n_detectors(0),
    no_detection(false),
    trial_detect_time(5.0),
    ccf_thr_idx(0),
    emg_thr_idx(0),
    ccf_det_idx(0),
    emg_det_idx(0),
    force_ready(false),
    acqUpdateCnt(0),
    trainTriggered(false),
    trialDetectBinCnt(0),
    isLastStimPain(true),
    greedy_cnt(0),
    plot_cnt(0),
    cnf_cnt(0),
    zplots(),
    decoding_time_s(0),
    training_cnt(0),
    c_scaling_factor(0)

  // **** to be done ****
{
}

void PainDetectClient::closeEvent (QCloseEvent *event)
{
    QMessageBox::StandardButton resBtn = QMessageBox::question( this, "PainDetectionClient",
                                                                tr("Are you sure?\n"),
                                                                QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes,
                                                                QMessageBox::Yes);
    if (resBtn != QMessageBox::Yes) {
        event->ignore();
    } else {
        collectTimer->stop();
        refreshTimer->stop();
        event->accept();
    }
}

PainDetectClient::~PainDetectClient()
{
    //destroy();
}

void PainDetectClient::init(std::string filename)
{
    // read configurations from .ini file
    conf.updateConfig(filename);
    // construct collector
    collector = new PlexDataCollector(*conf.acqConfig);

    std::cout << "EMG channels = "<< collector->get_emg_chn()[0]<<std::endl;

    // construct algorithm manager
    alg = new AlgorithmManager(conf.algConfig);
    // print decoders and detectors
    for (int i=0;i<alg->get_decoder_list().size();i++)
    {
        std::cout <<"decoder name="<< alg->get_decoder_list()[i]->get_name();
        std::cout <<" decoder type="<< alg->get_decoder_list()[i]->get_type();
        std::cout <<std::endl;
    }
    for (int i=0;i<alg->get_detector_list().size();i++)
    {
        std::cout <<"detector name="<< alg->get_detector_list()[i]->get_name();
        std::cout <<" detector type="<< alg->get_detector_list()[i]->get_type();
        std::cout <<" detector type="<< alg->get_detector_list()[i]->get_decoder_type();
        std::cout <<std::endl;
    }

    // construct digital output manager
    pDo = new PlexDo(conf.doConfig);
    // print DO channels
    std::cout <<" size="<<pDo->get_signal_list().size()<<std::endl;
    for (int i=0;i<pDo->get_signal_list().size();i++)
    {
        std::string sig = pDo->get_signal_list()[i];
        std::cout <<"i="<<i<<" signal name="<< sig;
        std::cout <<" chn ="<< pDo->get_do_chn(sig);
        std::cout <<std::endl;
    }

    // Digital output should be initialized before using
    pDo->init();

    // init training threads for each single detector (only PLDS/TLDS requires training)
    // each single model based detector ocupies a thread
    for (int i=0;i<alg->get_detector_list().size();i++)
    {
        if (alg->get_detector_list()[i]->get_type().compare("Single")==0)
        {
            if (alg->get_detector_list()[i]->get_decoder_type().compare("PLDS")==0 ||
                alg->get_detector_list()[i]->get_decoder_type().compare("TLDS")==0)
            {
                threads.push_back(new ThreadManager(Training,alg,collector,ui,pDo,alg->get_detector_list()[i]->get_name()));
                std::cout<<"new thread,detector name:"<<alg->get_detector_list()[i]->get_name()<<std::endl;
            }
        }
    }

    // init do threads
    threads.push_back(new ThreadManager(Laser,alg,collector,ui,pDo));
    threads.push_back(new ThreadManager(Opto,alg,collector,ui,pDo));
    // trigger thread is legacy
    //threads.push_back(new ThreadManager(Trigger,alg,collector,pDo));

    // init detection threads
    // each detection digital output signal is sent by independent threads to avoid mutual block
    n_detectors = 0;
    for (int i=0; i<pDo->get_signal_list().size();i++)
    {
        std::string tmp = pDo->get_signal_list()[i];
        tmp = tmp.substr(0,tmp.size()-1);
        std::cout << tmp<<std::endl;
        if (tmp.compare("detect")==0)
        {
            threads.push_back(new ThreadManager(Detection,alg,collector,ui,pDo,"",pDo->get_signal_list()[i]));
            std::cout<<"new thread, signal name:"<<pDo->get_signal_list()[i]<<std::endl;
            n_detectors++;
        }
    }

    // init data collector
    // data collector has to be initialized before using
    if (0==collector->init())
    {
        std::cout << "collector successfully initialized"<<std::endl;
    }
    else
    {
        std::cout << "collector initialization failed"<<std::endl;
        return;
    }

    // draw UI plots and wigets
    if (0==makePlot())
    {
        // start online data colletion and ploting
        start();
    }
    else return;
}

void PainDetectClient::start()
{
    // set collector start flag
    collector->start();

    // set the thread priority of collecting and decoding to be higher than others
    (*this).thread()->setPriority(QThread::HighestPriority);

    return;
}

int PainDetectClient::addTableRow(QString type, QString region)
{
    int rows = ui->tableWidget->rowCount();
    rows++;
    ui->tableWidget->setRowCount(rows);
    ui->tableWidget->setItem(rows-1, 0, new QTableWidgetItem(type));
    ui->tableWidget->setItem(rows-1, 1, new QTableWidgetItem(region));
    QTableWidgetItem *item2 = new QTableWidgetItem(type);
    item2->setCheckState(Qt::Unchecked);
    ui->tableWidget->setItem(rows-1, 0, item2);
    return rows-1;
}

void PainDetectClient::destroy()
{
    // destroy qthreads;
    for (int i=0;i<threads.size();i++)
    {
        if (threads[i]->isRunning())
        {
            threads[i]->abort();
            if(!threads[i]->wait(2000)) //Wait until it actually has terminated (max. 5 sec)
            {
                threads[i]->terminate(); //Thread didn't exit in time, probably deadlocked, terminate it!
                threads[i]->wait(); //We have to wait again here!
            }
        }
        delete threads[i];
    }

    // delete from the latest constructed object
    if (refreshTimer!=nullptr)
        delete refreshTimer;
    refreshTimer = nullptr;

    if (collectTimer!=nullptr)
        delete collectTimer;
    collectTimer = nullptr;

    for (int i=0;i<detection_lines.size();i++)
    {
        if (detection_lines[i]!=nullptr)
            delete detection_lines[i];
        detection_lines[i] = nullptr;
    }

    for (int i=0;i<stim_event_lines.size();i++)
    {
        if (stim_event_lines[i]!=nullptr)
            delete stim_event_lines[i];
        stim_event_lines[i] = nullptr;
    }

    for (int i=0;i<zplotThreshLines.size();i++)
    {
        if (zplotThreshLines[i]!=nullptr)
            delete zplotThreshLines[i];
        zplotThreshLines[i] = nullptr;
    }

    for (int i=0;i<zplotTextLabels.size();i++)
    {
        if (zplotTextLabels[i]!=nullptr)
            delete zplotTextLabels[i];
        zplotTextLabels[i] = nullptr;
    }

    if (cbarLine!=nullptr)
        delete cbarLine;
    cbarLine = nullptr;

    if (Cbar!=nullptr)
        delete Cbar;
    Cbar = nullptr;

    if (rasterColorScale!=nullptr)
        delete rasterColorScale;
    rasterColorScale = nullptr;

    if (rasterLine!=nullptr)
        delete rasterLine;
    rasterLine = nullptr;

    for (int i=0;i<rasterTextLabels.size();i++)
    {
        if (rasterTextLabels[i]!=nullptr)
            delete rasterTextLabels[i];
        rasterTextLabels[i] = nullptr;
    }
    if (raster!=nullptr)
        delete raster;
    raster = nullptr;
    if (rasterRect!=nullptr)
        delete rasterRect;
    rasterRect = nullptr;

    if (pDo != nullptr)
        delete pDo;
    pDo = nullptr;

    // destroy alg
    if (alg != nullptr)
        delete alg;
    alg = nullptr;
    // destroy collector
    if (collector != nullptr)
        delete collector;
    collector = nullptr;
}
void PainDetectClient::collect()
{
    //std::cout<< "collect timeout"<<std::endl;
    collector->collect();
}

/*
    update raster plots data for new data and current key(time)
*/
void PainDetectClient::updateRasterPlot(double key,string_vec regions,Eigen::VectorXf spikes,int region_idx,int unit_idx_shift)
{
    int value;
    int i = region_idx;
    if (key>plotTime)
    {
        if (regions.size()-1==i)//update cells and x range, only once
        {
            for (int i=0;i<raster->data()->keySize();i++)
            {
                for (int j=0;j<raster->data()->valueSize();j++)
                {
                    value = raster->data()->cell(i+1,j);
                    raster->data()->setCell(i,j,value);
                }
            }
            raster->data()->setKeyRange(QCPRange(key-8,key));
        }

        //update new data
        for (unsigned int j=0;j<collector->getnUnits(i);j++)
        {
            value = spikes[j];
            raster->data()->setCell(raster->data()->keySize()-1,j+unit_idx_shift+1,value);
        }
    }
    else
    {
        for (unsigned int j=0;j<collector->getnUnits(i);j++)
        {
            raster->data()->setData(key,j+unit_idx_shift+1,spikes[j]);
        }
    }
}

void PainDetectClient::updateTrialNumbers()
{
    QString str = QString::number(event_cnt[0])+"/"+QString::number(event_cnt_fp[0]);
    ui->label_8->setText(str);
}

void PainDetectClient::processHlaser(double key,int region_idx)
{
    int i = region_idx;
    if (0==i)
    {
        event_cnt[0]++;
        if (updateMode==auto_trial)
        {
            std::cout<<"auto training triggered"<<std::endl;
            acqUpdateCnt = 0;
            trainTriggered = true;
        }
        else
        {
            acqUpdateCnt = 0;
            std::cout<<"counter triggered"<<std::endl;
            //cntTriggered = true;
        }

        trialDetectBinCnt = 0;
        isLastStimPain = true;

        //update stim event on CCF/EMG plot
        int idx_ccf = collector->getRegion().size();
        //std::cout << "stim_event_lines.size()=="<<stim_event_lines.size()<<std::endl;
        for (int ii=0;ii<2;ii++)
        {
            stim_event_lines[idx_ccf+ii]->start->setCoords(key,0);
            stim_event_lines[idx_ccf+ii]->end->setCoords(key,1);
            stim_event_lines[idx_ccf+ii]->setVisible(true);
            stim_event_lines[idx_ccf+ii]->setPen(QPen(Qt::white, 2.0, Qt::SolidLine));
        }
    }
    updateTrialNumbers();
    updateTableEvnDetCnt();
    stim_event_lines[i]->start->setCoords(key,0);
    stim_event_lines[i]->end->setCoords(key,1);
    stim_event_lines[i]->setVisible(true);
    stim_event_lines[i]->setPen(QPen(Qt::white, 2.0, Qt::SolidLine));
}

void PainDetectClient::processLlaser(double key,int region_idx)
{
    int i = region_idx;
    if (0==i)
    {
        event_cnt_fp[0]++;

        acqUpdateCnt = 0;
        std::cout<<"counter triggered"<<std::endl;
        //cntTriggered = true;

        trialDetectBinCnt = 0;
        isLastStimPain = false;

        //update stim event on CCF/EMG plot
        int idx_ccf = collector->getRegion().size();
        for (int ii=0;ii<2;ii++)
        {
            stim_event_lines[idx_ccf+ii]->start->setCoords(key,0);
            stim_event_lines[idx_ccf+ii]->end->setCoords(key,1);
            stim_event_lines[idx_ccf+ii]->setVisible(true);
            stim_event_lines[idx_ccf+ii]->setPen(QPen(Qt::white, 2.0, Qt::DashLine));
        }

    }
    updateTrialNumbers();
    updateTableEvnDetCnt();
    stim_event_lines[i]->start->setCoords(key,0);
    stim_event_lines[i]->end->setCoords(key,1);
    stim_event_lines[i]->setVisible(true);
    stim_event_lines[i]->setPen(QPen(Qt::white, 2.0, Qt::DashLine));
}

/*
 * process DIN events to update flags/plots
*/
void PainDetectClient::processEvents(double key,int region_idx)
{
    int i = region_idx;
    if (collector->get_eventQueue().size()>0)
    {
        for (int j=0;j<collector->get_eventQueue().size();j++)
        {
            std::cout << "event["<<j<<"]="<<collector->get_eventQueue()[j]<<std::endl;
            if (collector->get_eventQueue()[j].compare("h_laser_on")==0)
            {
                processHlaser(key,i);
            }
            if (collector->get_eventQueue()[j].compare("l_laser_on")==0)
            {
                processLlaser(key,i);
            }
        }
    }
}

// should only be called when detection happened
void PainDetectClient::updateDetectionCnt(int det_idx)
{
    if (isLastStimPain)
    {
        // if current bin (detection happened time) is within *trial_detect_bins*
        // after stimulation, then count this detection as TP, otherwise count as FP
        if (trialDetectBinCnt<trial_detect_bins)
        {
            detection_cnt[det_idx]++;
        }
        else
        {
            detection_cnt_fp[det_idx]++;
        }
    }
    else
    {
        detection_cnt_fp[det_idx]++;
    }

    // trigger opto if checked
    QTableWidgetItem *item =  ui->tableWidget->item(det_idx,0);
    if(item->checkState()==Qt::Checked && optoTriggerMode==auto_trial)
    {
        std::cout << "opto triggered by detector #"<<det_idx<<std::endl;
        sendOpto();
    }

    updateTableEvnDetCnt();
}

void PainDetectClient::processEMG(EMGDetector* p_emg, double key)
{
    p_emg->clearDoneFlag();
    float emg_value = collector->read_emg();   //read emg
    float emg_mv = p_emg->get_emg_mv(emg_value); // digitize factor value to be verified
    bool emg_detected = p_emg->detect(emg_mv);
    ui->EMG_plot->graph(0)->addData(key,emg_mv);
}

void PainDetectClient::processCCF(CCFDetector* p_ccf, double key,int det_idx)
{
    p_ccf->clearDoneFlag();
    Eigen::VectorXf dummy;
    bool detected = p_ccf->detect(dummy);

    // add data to plot
    float x = p_ccf->get_ccf_value();
    ui->CCF_plot->graph(0)->addData(key,x);

    // update thresholds
    std::vector<float> thresh_ccf = p_ccf->get_ccf_thresh();
    for (int i=0;i<2;i++)
    {
        zplotThreshLines[ccf_thr_idx+i]->start->setType(QCPItemPosition::ptAxisRectRatio);
        zplotThreshLines[ccf_thr_idx+i]->start->setCoords(0, 1-(thresh_ccf[i]+4)/10);
        zplotThreshLines[ccf_thr_idx+i]->end->setType(QCPItemPosition::ptAxisRectRatio);
        zplotThreshLines[ccf_thr_idx+i]->end->setCoords(1, 1-(thresh_ccf[i]+4)/10);
        zplotThreshLines[ccf_thr_idx+i]->setPen(QPen(Qt::white, 2.0, Qt::DashLine));
    }

    updateTableCCFthresh();
    if (detected  && detection_cnt_ready)
    {
        if (det_idx<n_detectors)
        {
            std::cout << "detect"+std::to_string(det_idx+1)<<" sent"<<std::endl;
            sendDetection("detect"+std::to_string(det_idx+1));
        }

        updateDetectionCnt(det_idx);

        detection_lines[ccf_det_idx]->start->setCoords(key, -9);
        detection_lines[ccf_det_idx]->end->setCoords(key, 9);
        detection_lines[ccf_det_idx]->setVisible(true);

        std::cout <<p_ccf->get_name()<<"CCF "<<"detected key="<<key<<std::endl;
    }
}

void PainDetectClient::processGreedy(GreedyDetector* p_greedy,double key,int det_idx,std::vector<Eigen::VectorXf>& spike_vec, string_vec regions)
{
    p_greedy->clearDoneFlag();
    bool detected = p_greedy->detect(spike_vec,regions);
    if (detected  && detection_cnt_ready)
    {
        if (det_idx<n_detectors)
        {
            std::cout << "detect"+std::to_string(det_idx+1)<<" sent"<<std::endl;
            sendDetection("detect"+std::to_string(det_idx+1));
        }
        updateDetectionCnt(det_idx);
        for(int k=0;k<regions.size();k++)
        {
            detection_lines[k*n_detectors+det_idx]->start->setCoords(key, 0);
            detection_lines[k*n_detectors+det_idx]->end->setCoords(key, 0.8-greedy_cnt*0.1);
            detection_lines[k*n_detectors+det_idx]->setVisible(true);
        }
        std::cout <<p_greedy->get_name()<<"Greedy "<<"detected key="<<key<<std::endl;
    }
    greedy_cnt++;
}

void PainDetectClient::processMF(ModelFreeDetector* p_mf,double key,int det_idx,Eigen::VectorXf &spikes, string_vec regions, int region_idx)
{
    int i = region_idx;
    if (p_mf->get_region().compare(regions[i])==0)
    {
        p_mf->clearDoneFlag();
        bool detected = p_mf->detect(spikes);
        if (detected  && detection_cnt_ready)
        {
            if (det_idx<n_detectors)
            {
                std::cout << "detect"+std::to_string(det_idx+1)<<" sent"<<std::endl;
                sendDetection("detect"+std::to_string(det_idx+1));
            }
            updateDetectionCnt(det_idx);
            detection_lines[i*n_detectors+det_idx]->start->setCoords(key, 0);
            detection_lines[i*n_detectors+det_idx]->end->setCoords(key, 1);
            detection_lines[i*n_detectors+det_idx]->setVisible(true);
            std::cout <<regions[i]<<" MF " <<"detected det_idx="<<det_idx<<std::endl;
        }
        float x = p_mf->getX();
        //std::cout <<regions[i]<<" MF " <<"detected n_conf="<<n_conf<<" plot_cnt="<<plot_cnt<<std::endl;
        zplots[i]->graph(n_conf+plot_cnt)->addData(key,x);
        plot_cnt++;
    }
}

void PainDetectClient::processModel(ModelDetector* pdc, double key,int det_idx,Eigen::VectorXf &spikes, int region_idx, int unit_idx_shift)
{
    std::string decoderType = pdc->get_decoder_type();
    pdc->clearDoneFlag();
    int i = region_idx;
    bool detected = pdc->detect(spikes);
    if (detected && detection_cnt_ready)
    {
        if (det_idx<n_detectors)
        {
            std::cout << "detect"+std::to_string(det_idx+1)<<" sent"<<std::endl;
            sendDetection("detect"+std::to_string(det_idx+1));
        }

        updateDetectionCnt(det_idx);
        detection_lines[i*n_detectors+det_idx]->start->setCoords(key, 0);
        detection_lines[i*n_detectors+det_idx]->end->setCoords(key, 1);
        detection_lines[i*n_detectors+det_idx]->setVisible(true);
        //std::cout <<regions[i]<<" "<<decoderType <<"detected  key="<<key<<std::endl;
    }
    //get decoding value
    float z = pdc->getZscore();
    float conf = pdc->getConf();
    zplots[i]->graph(cnf_cnt)->addData(key,z+conf);
    cnf_cnt++;
    zplots[i]->graph(cnf_cnt)->addData(key,z-conf);
    cnf_cnt++;
    zplots[i]->graph(n_conf+plot_cnt)->addData(key,z);
    plot_cnt++;
    model_detector_status last_status = pdc->getStatus();
    pdc->process_based_on_status(spikes);

    if (decoderType.compare("PLDS")==0 && last_status==update_required
            && pdc->getStatus()==normal)
    {
        //update CBar
        auto bar_data = pdc->getDecoderC();
        float max_abs = 0;
        // get max abs value of C from the first region
        if (unit_idx_shift==0)
        {
            for (int j=0;j<bar_data.rows();j++)
            {
                cbarData[j+unit_idx_shift] =bar_data[j];
                max_abs = abs(bar_data[j])>max_abs?abs(bar_data[j]):max_abs;
            }
            c_scaling_factor = max_abs;
        }
        else
        {
            for (int j=0;j<bar_data.rows();j++)
            {
                max_abs = abs(bar_data[j])>max_abs?abs(bar_data[j]):max_abs;
            }
            float scaling_factor = c_scaling_factor/max_abs;
            for (int j=0;j<bar_data.rows();j++)
            {
                cbarData[j+unit_idx_shift] =bar_data[j]*scaling_factor;
            }
        }

        std::cout <<"Model Update"<<std::endl;
        //std::cout <<pdc->get_name()<<" "<<regions[i]<<" C="<<bar_data<<std::endl;
        Cbar->setData(cbarx, cbarData);
        ui->Cbar->rescaleAxes();
        ui->Cbar->yAxis->setRange((QCPRange(-1.5,collector->getnUnits())));
        updateTableBaseline(det_idx,pdc->get_meanBase(),pdc->get_stdBase());
        // set to updating status
        detection_cnt_ready = false;

        ui->label_14->setText("Updating");
        QPalette palette = ui->label_14->palette();
        palette.setColor(ui->label_14->foregroundRole(), Qt::red);
        ui->label_14->setPalette(palette);

        training_cnt--;
        std::cout << "training_cnt="<<training_cnt<<std::endl;
        if (training_cnt==0)
        {
            ui->label_10->setText("Not Training");
            QPalette palette = ui->label_10->palette();
            palette.setColor(ui->label_10->foregroundRole(), Qt::black);
            ui->label_10->setPalette(palette);
        }
    }
}

void PainDetectClient::processSingle(DetectorBase* pd, double key,int det_idx,Eigen::VectorXf &spikes, int region_idx,int unit_idx_shift, string_vec regions)
{
    int i = region_idx;
    std::string decoderType = pd->get_decoder_type();
    if (decoderType.compare("PLDS")==0 || decoderType.compare("TLDS")==0)
    {
        ModelDetector* pdc = static_cast<ModelDetector*>(pd);
        if (pdc->get_region().compare(regions[i])==0)
        {
            processModel(pdc,key,det_idx,spikes,i,unit_idx_shift);
        }

    }
    else if (decoderType.compare("ModelFree")==0)
    {
        ModelFreeDetector* p_mf = static_cast<ModelFreeDetector*>(pd);
        processMF(p_mf,key,det_idx,spikes,regions,i);
    }
}

/*
    loop over all the detectors do detection/decoding
*/
void PainDetectClient::processDetectors(double key, std::vector<Eigen::VectorXf>& spike_vec, Eigen::VectorXf &tmpSpike, string_vec &regions,int region_idx,int unit_idx_shift)
{
    int i = region_idx;
    //decode in main thread
    for (auto iter = alg->get_detector_list().begin();iter<alg->get_detector_list().end();iter++)
    {
        int det_idx = iter-alg->get_detector_list().begin();
        std::string detectorType = (*iter)->get_type();
        // draw single region deocding plots
        if (detectorType.compare("Single")==0)
        {
            processSingle(*iter,key,det_idx,tmpSpike,i,unit_idx_shift,regions);
        }
        // draw greedy detecting results
        else if (detectorType.compare("Greedy")==0 && 0==i)
        {
            GreedyDetector* p_greedy = static_cast<GreedyDetector*>(*iter);
            processGreedy(p_greedy,key,det_idx,spike_vec,regions);
        }
        else if (detectorType.compare("CCF")==0 && 0==i)
        {
            CCFDetector* p_ccf = static_cast<CCFDetector*>(*iter);
            processCCF(p_ccf,key,det_idx);
        }
        else if (detectorType.compare("EMG")==0 && 0==i)
        {
            EMGDetector* p_emg = static_cast<EMGDetector*>(*iter);
            processEMG(p_emg,key);
        }

    }
}

void PainDetectClient::processFlags()
{
    // check opto status
    if (pDo->isOptoOn())
    {
        ui->label_11->setText("Opto ON");
        QPalette palette = ui->label_11->palette();
        palette.setColor(ui->label_11->foregroundRole(), Qt::green);
        ui->label_11->setPalette(palette);
    }
    else
    {
        ui->label_11->setText("Opto OFF");
        QPalette palette = ui->label_11->palette();
        palette.setColor(ui->label_11->foregroundRole(), Qt::red);
        ui->label_11->setPalette(palette);
    }

    // check if no detector detected any change in last 100 bins
    no_detection = true;
    for (auto iter = alg->get_detector_list().begin();iter<alg->get_detector_list().end();iter++)
    {
        if ((*iter)->get_last_detect_cnt()<100)
        {
            no_detection = false;
            break;
        }
    }

    // if CCF exist, only look at CCF detection results
    for (auto iter = alg->get_detector_list().begin();iter<alg->get_detector_list().end();iter++)
    {
        if ((*iter)->get_type().compare("CCF")==0)
        {
            no_detection = true;
            if ((*iter)->get_last_detect_cnt()<100)
            {
                no_detection = false;
                break;
            }
        }
    }

    // if *no detection* or *force to be ready*, delivering stimulus is allowed
    alg->ready = no_detection;
    if (force_ready)
        alg->ready = true;

    if (alg->ready)
    {
        ui->label_46->setText("Ready");
        QPalette palette = ui->label_46->palette();
        palette.setColor(ui->label_46->foregroundRole(), Qt::green);
        ui->label_46->setPalette(palette);
    }
    else
    {
        ui->label_46->setText("Wait");
        QPalette palette = ui->label_46->palette();
        palette.setColor(ui->label_46->foregroundRole(), Qt::red);
        ui->label_46->setPalette(palette);
    }

    // cnt 100 bins after triggered for training, to use the data around stimulation
    if (acqUpdateCnt>100 && trainTriggered)
    {
        for (int i=0;i<threads.size();i++)
        {
            if (threads[i]->getType()==Training && !threads[i]->isRunning())
            {
                acqUpdateCnt = 0;
                threads[i]->set_ui(ui);
                threads[i]->start(QThread::LowestPriority);
                training_cnt++;
            }
            else
                std::cout <<"training is going on: "<<threads[i]->get_detector()<<std::endl;
        }
        trainTriggered = false;
        acqUpdateCnt=0;
        ui->label_10->setText("Training");
        QPalette palette = ui->label_10->palette();
        palette.setColor(ui->label_10->foregroundRole(), Qt::red);
        ui->label_10->setPalette(palette);
    }

    //update trialDetectBinCnt
    if (trialDetectBinCnt<trial_detect_bins*2)
        trialDetectBinCnt++;

    if (acqUpdateCnt<101)
        acqUpdateCnt++;

    greedy_cnt = 0;
}

void PainDetectClient::refresh()
{
    static double last_key = 0;
    static QTime time(QTime::currentTime()); //Note that the accuracy depends on the accuracy of the underlying operating system; not all systems provide 1-millisecond accuracy.
    double key = time.elapsed()/1000.0;   //x axis coordinate

    trial_detect_bins = trial_detect_time/collector->getBinSize();

    string_vec regions = collector->getRegion();
    int x,y;
    int unit_idx_shift = 0;
    raster->data()->coordToCell(key, 1, &x, &y);
    std::vector<Eigen::VectorXf> spike_vec;

    if(optoTriggerMode == random)
    {
        static int count = 0;
        static double opto_time = 0;
        static int opto_key = 0;

        opto_time += (key-last_key);
        if (opto_time >= 1)
        {
            count++;
            opto_time = 0;
        }
        if ((count % random_opto_freq) == 0)
        {
            opto_key = 1 + ( std::rand() % random_opto_freq );
        }
        if ((count % random_opto_freq) + 1 == opto_key && opto_key != 0)
        {
            sendOpto();
            //std::cout << "random_opto_freq" << random_opto_freq << ".\n" << std::endl;
        }
        //std::cout << "current" << key << "pre" << last_key << "key" << opto_key << ".\n" << std::endl;

    }
    // update detection time
    if (detection_cnt_ready)
    {
        decoding_time_s += (key-last_key);
    }
    last_key = key;
    // update decoding time
    //QString str = QString::number(decoding_time_s) + "(s)";
    QString str = QDateTime::fromTime_t(decoding_time_s).toUTC().toString("hh:mm:ss");
    ui->label_16->setText(str);

    // check if new spike data ready
    if (collector->isDataReady())
    {
        processFlags();
        spike_vec.clear();
        //because the smaller unit idx is at the lower part of the raster,
        //iterate over regions reversely to put the first region on the upper side
        //to avoid confusing
        for (int i=regions.size()-1;i>=0;i--)
        {
            cnf_cnt = 0;
            plot_cnt = 0;
            Eigen::VectorXf tmpSpike = collector->read(regions[i]);   //read spikes
            //std::cout << "Debug :  Row: " << tmpSpike.rows() << " Col: "<< tmpSpike.cols() << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
            for (int i=0; i<tmpSpike.rows(); i++)
            {
                QTableWidgetItem *item = ui->tableWidget_2->item(i, 0);
                if(item->checkState()==Qt::Unchecked)
                {
                    tmpSpike.row(i) << 0;
                }
            }
            spike_vec.insert(spike_vec.begin(),tmpSpike);

            //detect with all detectors
            processDetectors(key,spike_vec,tmpSpike,regions,i,unit_idx_shift);
            // process events
            processEvents(key,i);
            //update raster plot of current region
            updateRasterPlot(key,regions,tmpSpike,i,unit_idx_shift);

            unit_idx_shift = unit_idx_shift + collector->getnUnits(i);
        }

        // update zplots
        for (int i=regions.size()-1;i>=0;i--)
        {
            zplots[i]->xAxis->setRange(key, 8, Qt::AlignRight);
            zplots[i]->replot();
        }
        // update other plots
        ui->CCF_plot->xAxis->setRange(key, 8, Qt::AlignRight);
        ui->CCF_plot->replot();
        ui->EMG_plot->xAxis->setRange(key, 8, Qt::AlignRight);
        ui->EMG_plot->replot();
        ui->Cbar->replot();

        collector->resetReady();
    }

    // update axis
    ui->RasterPlot->xAxis->setRange(key, plotTime, Qt::AlignRight);
    ui->RasterPlot->replot();

    // to be done:
    // update status bar
}

int PainDetectClient::makePlot()
{
    ui->setupUi(this);
    // plot background
    QLinearGradient axisRectGradient;
    axisRectGradient.setColorAt(0, QColor(80, 80, 80));
    axisRectGradient.setColorAt(1, QColor(80, 80, 80));
    // set time ticker
    QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
    timeTicker->setTimeFormat("%h:%m:%s");

    /********************************************************************
    ************************ setup raster plot **************************
    *********************************************************************/
    rasterRect = new QCPAxisRect(ui->RasterPlot);
    ui->RasterPlot->plotLayout()->clear();
    ui->RasterPlot->plotLayout()->addElement(0, 0, rasterRect);

    int nx = round(plotTime/collector->getBinSize());//number of showed bins
    int ny = 0;
    int ny_all = collector->getnUnits();
    float ratio = 0;

    raster = new QCPColorMap(rasterRect->axis(QCPAxis::atBottom), rasterRect->axis(QCPAxis::atLeft));
    ui->RasterPlot->setInteractions(QCP::iRangeDrag|QCP::iRangeZoom); // this will also allow rescaling the color scale by dragging/zooming
    ui->RasterPlot->axisRect()->setupFullAxesBox(true);
    ui->RasterPlot->xAxis->setTicker(timeTicker);
    ui->RasterPlot->yAxis->setRange(0, ny_all+1);

    raster->data()->setSize(nx, ny_all+2); // we want the color map to have nx * ny data points
    raster->data()->setRange(QCPRange(0, 8), QCPRange(0, ny_all+1));
    std::cout<<"key size="<<raster->data()->keySize()<<std::endl;

    string_vec regions = collector->getRegion();

    // for now the maximum number of regions = 2
    if (1==regions.size())
    {
        ny = ny_all;
        ratio = 0;
        rasterLine = nullptr;
        // setup label for number of units in each region
        ui->label_2->setText(QString::fromStdString(regions[0])+": "+QString::number(ny_all));
    }
    else if (2==regions.size())
    {
        ny = collector->getnUnits(0);
        ratio = float(ny)/float(ny_all);

        //add line between two regions
        rasterLine = new QCPItemLine(ui->RasterPlot);
        rasterLine->start->setType(QCPItemPosition::ptAxisRectRatio);
        rasterLine->start->setCoords(0, ratio);
        rasterLine->end->setType(QCPItemPosition::ptAxisRectRatio);
        rasterLine->end->setCoords(1, ratio);
        rasterLine->setPen(QPen(Qt::red,2.0));

        // setup label for number of units in each region
        ui->label_2->setText(QString::fromStdString(regions[0])+": "+QString::number(ny));
        ui->label_3->setText(QString::fromStdString(regions[1])+": "+QString::number(ny_all-ny));
    }
    else
    {
        std::cout << "invalid number of regions = "<<regions.size()<<std::endl;
        return -1;
    }

    // add region name text labels
    for (int i=0;i<regions.size();i++)
    {
        rasterTextLabels.push_back(new QCPItemText(ui->RasterPlot));
        rasterTextLabels[i]->setPositionAlignment(Qt::AlignTop|Qt::AlignHCenter);
        rasterTextLabels[i]->position->setType(QCPItemPosition::ptAxisRectRatio);
        rasterTextLabels[i]->position->setCoords(0.2, i*ratio); // place position at center/top of axis rect
        rasterTextLabels[i]->setText(QString::fromStdString(regions[i]));
        rasterTextLabels[i]->setFont(QFont(font().family(), 24)); // make font a bit larger
        rasterTextLabels[i]->setColor(QColor(255*(1-i),255*i,0));
    }

    // add a color scale
    rasterColorScale = new QCPColorScale(ui->colorscale);
    ui->colorscale->plotLayout()->clear();
    ui->colorscale->plotLayout()->addElement(0,0,rasterColorScale);
    rasterColorScale->setType(QCPAxis::atBottom);

    raster->setColorScale(rasterColorScale); // associate the color map with the color scale
    rasterColorScale->axis()->setLabel("Spike count");

    // set the color gradient of the color map to one of the presets:
    QCPColorGradient QCPcg(QCPColorGradient::gpNight);
    raster->setGradient(QCPcg);
    //ACCraster->data()->fillAlpha(64);

    raster->setInterpolate(false);
    raster->setTightBoundary(false);
    ui->RasterPlot->setBackground(axisRectGradient);

    std::cout << "raster plot done "<<std::endl;

    /********************************************************************
    ****************** setup bar plot for C vector **********************
    *********************************************************************/
    cbarData = QVector<double>(ny_all);
    cbarx = QVector<double>(ny_all);
    for (int i=0; i<cbarData.size(); ++i)
    {
        cbarx[i] = i;
        cbarData[i] = 0;// init with all zeroes
    }
    Cbar = new QCPBars(ui->Cbar->yAxis, ui->Cbar->xAxis);
    Cbar->setData(cbarx, cbarData);
    Cbar->setPen(Qt::NoPen);
    Cbar->setBrush(QColor(10, 140, 70, 160));
    ui->Cbar->xAxis->setVisible(false);
    ui->Cbar->yAxis->setVisible(false);
    ui->Cbar->xAxis->grid()->setPen(Qt::NoPen);
    ui->Cbar->yAxis->grid()->setPen(Qt::NoPen);
    ui->Cbar->xAxis->grid()->setSubGridPen(Qt::NoPen);
    ui->Cbar->rescaleAxes();
    ui->Cbar->yAxis->setRange((QCPRange(-1.5,ny_all)));
    ui->Cbar->xAxis->setTickLabelFont(QFont(fontInfo().family(),14,QFont::Bold));
    ui->Cbar->addLayer("belowmain", ui->Cbar->layer("main"), QCustomPlot::limBelow);
    ui->Cbar->xAxis->grid()->setLayer("belowmain");
    ui->Cbar->yAxis->grid()->setLayer("belowmain");
    ui->Cbar->setBackground(axisRectGradient);

    //C bar plot line
    if (2==regions.size())
    {
        //ny = collector->getnUnits(0);
        ratio = float(ny+0.5)/float(ny_all+1.5);
        //add line between two regions
        cbarLine = new QCPItemLine(ui->Cbar);
        cbarLine->start->setType(QCPItemPosition::ptAxisRectRatio);
        cbarLine->start->setCoords(0, ratio);
        cbarLine->end->setType(QCPItemPosition::ptAxisRectRatio);
        cbarLine->end->setCoords(1, ratio);
        cbarLine->setPen(QPen(Qt::red,2.0));
    }

    std::cout << "Cbar plot done "<<std::endl;
    /********************************************************************
    ************************ setup z-score plots ************************
    *********************************************************************/
    //std::vector<QCustomPlot*> zplots;
    zplots.push_back(ui->ZscorePlot_up);
    zplots.push_back(ui->ZscorePlot_down);

    if (1==regions.size())
    {
        //disable the lower plot
        ui->ZscorePlot_down->setVisible(false);
    }
    else if (2==regions.size())
    {
        // just do nothing
    }
    else
    {
        std::cout << "invalid number of regions = "<<regions.size()<<std::endl;
        return -1;
    }

    //=======================================================

    for (int rows=1; rows <= ny_all; ++rows)
    {
        int row = ui->tableWidget_2->rowCount();
        row++;
        ui->tableWidget_2->setRowCount(row);
        ui->tableWidget_2->setItem(row-1, 0, new QTableWidgetItem(QString::fromStdString(std::to_string(row))));
        QTableWidgetItem *item2 = new QTableWidgetItem();
        item2->setCheckState(Qt::Checked);
        ui->tableWidget_2->setItem(row-1, 0, item2);
        ui->tableWidget_2->setRowHeight(row-1, 20);
    }
    //=======================================================

    // set up plots
    for (int i=0;i<regions.size();i++)
    {
        zplots[i]->xAxis->setTicker(timeTicker);
        zplots[i]->axisRect()->setupFullAxesBox();
        zplots[i]->yAxis->setRange(-4, 6);
        connect(zplots[i]->xAxis, SIGNAL(rangeChanged(QCPRange)), zplots[i]->xAxis2, SLOT(setRange(QCPRange)));
        zplots[i]->xAxis->setTickLabelFont(QFont(fontInfo().family(),14,QFont::Bold));
        zplots[i]->yAxis->setTickLabelFont(QFont(fontInfo().family(),14,QFont::Bold));
        zplots[i]->xAxis->setTickLabels(false);// remove tick labels
        zplots[i]->legend->setVisible(true);
        zplots[i]->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignLeft|Qt::AlignTop);
        zplots[i]->setBackground(axisRectGradient);
        zplots[i]->xAxis->grid()->setPen(Qt::NoPen);// remove plot grids
        zplots[i]->yAxis->grid()->setPen(Qt::NoPen);
    }

    double default_thresh[3];
    default_thresh[0] = 1-(1.65+4.0)/10.0;
    default_thresh[1] = 1-(-1.65+4.0)/10.0;
    default_thresh[2] = 1-(3.38+4.0)/10.0;// model-free thresh
    std::vector<Qt::PenStyle> threshPen;
    threshPen.push_back(Qt::DashLine);
    threshPen.push_back(Qt::DashLine);
    threshPen.push_back(Qt::DashLine);
    int line_cnt = 0;
    // add text and line items
    for (int i=0;i<regions.size();i++)
    {
        // add region name text labels
        zplotTextLabels.push_back(new QCPItemText(zplots[i]));

        zplotTextLabels[i]->setPositionAlignment(Qt::AlignTop|Qt::AlignHCenter);
        zplotTextLabels[i]->position->setType(QCPItemPosition::ptAxisRectRatio);
        zplotTextLabels[i]->position->setCoords(0.2, 0); // place position at center/top of axis rect
        zplotTextLabels[i]->setText(QString::fromStdString(regions[i]));
        zplotTextLabels[i]->setFont(QFont(font().family(), 24)); // make font a bit larger
        zplotTextLabels[i]->setColor(QColor(255*(1-i),255*i,0));

        //add thresh lines
        //for (int j=0;j<3;j++)
        for (int j=0;j<2;j++)
        {
            zplotThreshLines.push_back(new QCPItemLine(zplots[i]));

            zplotThreshLines[line_cnt]->start->setType(QCPItemPosition::ptAxisRectRatio);
            zplotThreshLines[line_cnt]->start->setCoords(0, default_thresh[j]);
            zplotThreshLines[line_cnt]->end->setType(QCPItemPosition::ptAxisRectRatio);
            zplotThreshLines[line_cnt]->end->setCoords(1, default_thresh[j]);
            zplotThreshLines[line_cnt]->setPen(QPen(Qt::white, 2.0, threshPen[j]));
            line_cnt++;
        }
    }

    std::cout << "z-score plot done "<<std::endl;
    /********************************************************************
    ************************ add z-score plots data *********************
    *********************************************************************/
    for (int i=0;i<regions.size();i++)
    {
        int cnf_cnt = 0;
        int plot_cnt = 0;
        // add confidence intervals
        for(int j=0;j<alg->get_detector_list().size();j++)
        {
            if (alg->get_detector_list()[j]->get_region().compare(regions[i])==0)
            {
                if (alg->get_detector_list()[j]->get_type().compare("Single")==0)
                {
                    if ((alg->get_detector_list()[j]->get_decoder_type().compare("PLDS")==0)
                            ||(alg->get_detector_list()[j]->get_decoder_type().compare("TLDS")==0) )
                    {
                        zplots[i]->addGraph();
                        zplots[i]->graph(cnf_cnt)->setPen(Qt::NoPen);
                        cnf_cnt++;
                        zplots[i]->addGraph();
                        zplots[i]->graph(cnf_cnt)->setPen(Qt::NoPen);
                        zplots[i]->graph(cnf_cnt)->setChannelFillGraph(zplots[i]->graph(cnf_cnt-1));
                        zplots[i]->graph(cnf_cnt)->setBrush(plot_colors_alpha[plot_cnt]);
                        //std::cout <<"type="<<alg->get_detector_list()[j]->get_type()<< "cnf zplots["<<i<<"]="<<cnf_cnt<<std::endl;
                        cnf_cnt++;
                    }
                    plot_cnt++;
                }
            }
        }

        zplots[i]->legend->clear();
        n_conf = cnf_cnt;
        // add data plots
        plot_cnt = 0;
        for(int j=0;j<alg->get_detector_list().size();j++)
        {
            if (alg->get_detector_list()[j]->get_region().compare(regions[i])==0)
            {
                //std::cout <<"type="<<alg->get_detector_list()[j]->get_type()<<std::endl;
                if (alg->get_detector_list()[j]->get_type().compare("Single")==0)
                {
                    zplots[i]->addGraph();
                    zplots[i]->graph(cnf_cnt+plot_cnt)->setPen(QPen(plot_colors[plot_cnt],3));
                    zplots[i]->graph(cnf_cnt+plot_cnt)->setName(QString::fromStdString(alg->get_detector_list()[j]->get_decoder_type()));
                    //std::cout <<"type="<<alg->get_detector_list()[j]->get_type()<< " zplots["<<i<<"]="<<cnf_cnt+plot_cnt<<std::endl;
                    plot_cnt++;
                }
            }
        }
    }

    std::cout << "z-score data done "<<std::endl;
    /********************************************************************
    ************************ setup CCF plot  ****************************
    *********************************************************************/
    ui->CCF_plot->setVisible(false);
    for(int j=0;j<alg->get_detector_list().size();j++)
    {
        if (alg->get_detector_list()[j]->get_type().compare("CCF")==0)
        {
            ui->CCF_plot->setVisible(true);
            // setup plots
            ui->CCF_plot->xAxis->setTicker(timeTicker);
            ui->CCF_plot->axisRect()->setupFullAxesBox();
            ui->CCF_plot->yAxis->setRange(-4, 6);
            connect(ui->CCF_plot->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->CCF_plot->xAxis2, SLOT(setRange(QCPRange)));
            ui->CCF_plot->xAxis->setTickLabelFont(QFont(fontInfo().family(),14,QFont::Bold));
            ui->CCF_plot->yAxis->setTickLabelFont(QFont(fontInfo().family(),14,QFont::Bold));
            ui->CCF_plot->xAxis->setTickLabels(false);// remove tick labels

            ui->CCF_plot->legend->setVisible(false);

            ui->CCF_plot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignLeft|Qt::AlignTop);
            ui->CCF_plot->setBackground(axisRectGradient);
            ui->CCF_plot->xAxis->grid()->setPen(Qt::NoPen);// remove plot grids
            ui->CCF_plot->yAxis->grid()->setPen(Qt::NoPen);

            // add region name text labels
            zplotTextLabels.push_back(new QCPItemText(ui->CCF_plot));
            int idx = zplotTextLabels.size()-1;

            zplotTextLabels[idx]->setPositionAlignment(Qt::AlignTop|Qt::AlignHCenter);
            zplotTextLabels[idx]->position->setType(QCPItemPosition::ptAxisRectRatio);
            zplotTextLabels[idx]->position->setCoords(0.2, 0); // place position at center/top of axis rect
            zplotTextLabels[idx]->setText(QString::fromStdString("CCF"));
            zplotTextLabels[idx]->setFont(QFont(font().family(), 24)); // make font a bit larger
            zplotTextLabels[idx]->setColor(QColor(Qt::white));

            //add thresh lines
            //CCFDetector* p_ccf = static_cast<CCFDetector*>(alg->get_detector_list()[j]);
            //std::vector<float> thresh_ccf = p_ccf->get_ccf_thresh();

            std::vector<float> default_thresh = {float(1-(-1+5)/10.0),float(1-(1+5)/10.0)};
            ccf_thr_idx = line_cnt;
            for (int i=0;i<2;i++)
            {
                zplotThreshLines.push_back(new QCPItemLine(ui->CCF_plot));
                zplotThreshLines[line_cnt]->start->setType(QCPItemPosition::ptAxisRectRatio);
                zplotThreshLines[line_cnt]->start->setCoords(0, default_thresh[i]);
                zplotThreshLines[line_cnt]->end->setType(QCPItemPosition::ptAxisRectRatio);
                zplotThreshLines[line_cnt]->end->setCoords(1, default_thresh[i]);
                zplotThreshLines[line_cnt]->setPen(QPen(Qt::white, 2.0, Qt::DashLine));
                line_cnt++;
            }

            // add CCF plots
            ui->CCF_plot->addGraph();
            ui->CCF_plot->graph(0)->setPen(QPen(plot_colors[0],3));
            ui->CCF_plot->graph(0)->setName("CCF");
            //ui->CCF_plot->addGraph();
            //ui->CCF_plot->graph(1)->setPen(QPen(plot_colors[1],3));
            //ui->CCF_plot->graph(1)->setName("Area");

            break;
        }
    }

    std::cout << "ccf plot done "<<std::endl;
    /********************************************************************
    ************************ setup EMG plot  ****************************
    *********************************************************************/
    // to be done
    ui->EMG_plot->setVisible(false);
    for(int j=0;j<alg->get_detector_list().size();j++)
    {
        if (alg->get_detector_list()[j]->get_type().compare("EMG")==0)
        {
            ui->EMG_plot->setVisible(true);

            ui->EMG_plot->xAxis->setTicker(timeTicker);
            ui->EMG_plot->axisRect()->setupFullAxesBox();
            ui->EMG_plot->yAxis->setRange(-4, 6);
            connect(ui->EMG_plot->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->EMG_plot->xAxis2, SLOT(setRange(QCPRange)));
            ui->EMG_plot->xAxis->setTickLabelFont(QFont(fontInfo().family(),14,QFont::Bold));
            ui->EMG_plot->yAxis->setTickLabelFont(QFont(fontInfo().family(),14,QFont::Bold));
            ui->EMG_plot->xAxis->setTickLabels(false);// remove tick labels
            ui->EMG_plot->legend->setVisible(false);
            ui->EMG_plot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignLeft|Qt::AlignTop);
            ui->EMG_plot->setBackground(axisRectGradient);
            ui->EMG_plot->xAxis->grid()->setPen(Qt::NoPen);// remove plot grids
            ui->EMG_plot->yAxis->grid()->setPen(Qt::NoPen);


            QCPItemText* pt = new QCPItemText(ui->EMG_plot);
            pt->setPositionAlignment(Qt::AlignTop|Qt::AlignHCenter);
            pt->position->setType(QCPItemPosition::ptAxisRectRatio);
            pt->position->setCoords(0.2, 0); // place position at center/top of axis rect
            pt->setText(QString::fromStdString("EMG"));
            pt->setFont(QFont(font().family(), 24)); // make font a bit larger
            pt->setColor(QColor(Qt::white));

            float thresh_emg = 1-(alg->get_detector_list()[j]->get_thresh()+4.0)/10.0;
            std::cout <<"thresh_emg="<<thresh_emg<<std::endl;
            emg_thr_idx = line_cnt;
            zplotThreshLines.push_back(new QCPItemLine(ui->EMG_plot));
            zplotThreshLines[line_cnt]->start->setType(QCPItemPosition::ptAxisRectRatio);
            zplotThreshLines[line_cnt]->start->setCoords(0, thresh_emg);
            zplotThreshLines[line_cnt]->end->setType(QCPItemPosition::ptAxisRectRatio);
            zplotThreshLines[line_cnt]->end->setCoords(1, thresh_emg);
            zplotThreshLines[line_cnt]->setPen(QPen(Qt::white, 2.0, Qt::DashLine));
            zplotThreshLines[line_cnt]->setVisible(true);
            line_cnt++;

            // add EMG plots
            ui->EMG_plot->addGraph();
            ui->EMG_plot->graph(0)->setPen(QPen(plot_colors[0],3));
            ui->EMG_plot->graph(0)->setName("EMG");

            std::cout << "emg plot done "<<std::endl;
        }
    }
    /********************************************************************
    ************************ setup table widget *************************
    *********************************************************************/
    ui->tableWidget->adjustSize();
    int cols = ui->tableWidget->columnCount();
    ui->tableWidget->setColumnWidth(0,140);
    for (int i=1;i<cols;i++)
    {
        ui->tableWidget->setColumnWidth(i,60);
    }
    QString det_type;
    QString det_region;
    float thresh;

    for (int i=0;i<alg->get_detector_list().size();i++)
    {
        thresh = 0;
        if (alg->get_detector_list()[i]->get_type().compare("Single")==0)
        {
            det_type = QString::fromStdString(alg->get_detector_list()[i]->get_decoder_type());
            det_region = QString::fromStdString(alg->get_detector_list()[i]->get_region());
            thresh =  alg->get_detector_list()[i]->get_thresh();
        }
        else if(alg->get_detector_list()[i]->get_type().compare("EMG")==0)
        {
            det_type = QString::fromStdString(alg->get_detector_list()[i]->get_decoder_type());
            det_region = "";
            thresh = alg->get_detector_list()[i]->get_thresh();
        }
        else
        {
            det_region.clear();
            det_type = QString::fromStdString(alg->get_detector_list()[i]->get_type());
            GreedyDetector* gdp = static_cast<GreedyDetector*>(alg->get_detector_list()[i]);
            for (int j=0;j<gdp->get_detector_list().size();j++)
            {
                for (int k=0;k<alg->get_detector_list().size();k++)
                {
                    if (gdp->get_detector_list()[j]->get_name().compare(alg->get_detector_list()[k]->get_name())==0)
                    {
                        det_region += QString::number(k+1)+",";
                    }
                }
            }
            det_region.chop(1);
            //det_region = QString::fromStdString(alg->get_detector_list()[i]->get_decoder_type());
        }
        addTableRow(det_type,det_region);
        if (thresh>0)
        {
            ui->tableWidget->setItem(i, 4, new QTableWidgetItem(QString::number(thresh)));
        }
    }

    std::cout << "table plot done "<<std::endl;
    /********************************************************************
    ************************ setup event lines  *************************
    *********************************************************************/

    //visualize stim events
    for (int i=0;i< collector->getRegion().size();i++)
    {
        stim_event_lines.push_back(new QCPItemLine(zplots[i]));
        stim_event_lines[i]->start->setTypeX(QCPItemPosition::ptPlotCoords);
        stim_event_lines[i]->start->setTypeY(QCPItemPosition::ptAxisRectRatio);
        stim_event_lines[i]->start->setCoords(0, 0);
        stim_event_lines[i]->end->setTypeX(QCPItemPosition::ptPlotCoords);
        stim_event_lines[i]->end->setTypeY(QCPItemPosition::ptAxisRectRatio);
        stim_event_lines[i]->end->setCoords(0, 1); // point to (4, 1.6) in x-y-plot coordinates
        stim_event_lines[i]->setPen(QPen(Qt::white, 2.0));
        stim_event_lines[i]->setVisible(false);
    }
    //CCF/EMG plot stim line
    std::vector<QCustomPlot*> nonzplots;
    nonzplots.push_back(ui->CCF_plot);
    nonzplots.push_back(ui->EMG_plot);
    int n_region = collector->getRegion().size();
    for (int i=0;i< nonzplots.size();i++)
    {
        stim_event_lines.push_back(new QCPItemLine(nonzplots[i]));
        stim_event_lines[i+n_region]->start->setTypeX(QCPItemPosition::ptPlotCoords);
        stim_event_lines[i+n_region]->start->setTypeY(QCPItemPosition::ptAxisRectRatio);
        stim_event_lines[i+n_region]->start->setCoords(0, 0);
        stim_event_lines[i+n_region]->end->setTypeX(QCPItemPosition::ptPlotCoords);
        stim_event_lines[i+n_region]->end->setTypeY(QCPItemPosition::ptAxisRectRatio);
        stim_event_lines[i+n_region]->end->setCoords(0, 1); // point to (4, 1.6) in x-y-plot coordinates
        stim_event_lines[i+n_region]->setPen(QPen(Qt::white, 2.0));
        stim_event_lines[i+n_region]->setVisible(false);
    }

    //visualize detection events
    int n_detectors = alg->get_detector_list().size();
    int single_cnt = 0;
    for (int j=0;j<n_detectors;j++)
    {
        if (alg->get_detector_list()[j]->get_type().compare("Single"))
            single_cnt++;
    }
    int n_colors = plot_colors.size();
    int n_styles = plot_penstyle.size();
    int region_single_cnt = 0;
    for (int i=0;i< collector->getRegion().size();i++)
    {
        region_single_cnt = 0;
        for (int j=0;j<n_detectors;j++)
        {
            detection_lines.push_back(new QCPItemLine(zplots[i]));
            detection_lines[i*n_detectors+j]->start->setTypeX(QCPItemPosition::ptPlotCoords);
            detection_lines[i*n_detectors+j]->start->setTypeY(QCPItemPosition::ptAxisRectRatio);
            detection_lines[i*n_detectors+j]->start->setCoords(0, 0);
            detection_lines[i*n_detectors+j]->end->setTypeX(QCPItemPosition::ptPlotCoords);
            detection_lines[i*n_detectors+j]->end->setTypeY(QCPItemPosition::ptAxisRectRatio);
            detection_lines[i*n_detectors+j]->end->setCoords(0, 1);
            int c_idx;
            int s_idx = 0;
            std::string type_ = alg->get_detector_list()[j]->get_type();
            if (type_.compare("Single")==0 && collector->getRegion()[i].compare(alg->get_detector_list()[j]->get_region())==0)
            {
                c_idx = region_single_cnt % n_colors;
                //s_idx = (region_single_cnt-c_idx) % n_styles;
                s_idx = 0;
                region_single_cnt++;
            }
            else
            {
                int tmp = j-single_cnt*(collector->getRegion().size()-1)+1;
                c_idx = tmp % n_colors;
                //s_idx = (tmp-c_idx) % n_styles;
                s_idx = 1;
            }
            std::cout<< "region_single_cnt="<<region_single_cnt<<" n_colors="<<n_colors<<std::endl;
            std::cout<<"detector="<<alg->get_detector_list()[j]->get_name()<< "c_idx="<<c_idx<<std::endl;
            detection_lines[i*n_detectors+j]->setPen(QPen(plot_colors[c_idx], 2.0,plot_penstyle[s_idx]));
            detection_lines[i*n_detectors+j]->setVisible(false);
        }
    }
    // detection line for CCF
    detection_lines.push_back(new QCPItemLine(ui->CCF_plot));
    ccf_det_idx = detection_lines.size()-1;
    detection_lines[detection_lines.size()-1]->setPen(QPen(plot_colors[0], 2.0,plot_penstyle[0]));
    detection_lines[detection_lines.size()-1]->setVisible(false);
    // detection line for EMG
    detection_lines.push_back(new QCPItemLine(ui->EMG_plot));
    emg_det_idx = detection_lines.size()-1;
    detection_lines[detection_lines.size()-1]->setPen(QPen(plot_colors[0], 2.0,plot_penstyle[0]));
    detection_lines[detection_lines.size()-1]->setVisible(false);

    std::cout << "event plot done "<<std::endl;
    /********************************************************************
    ************************ setup event/detection **********************
    *********************************************************************/
    detection_cnt = std::vector<int>(alg->get_detector_list().size(),0);
    event_cnt = std::vector<int>(collector->get_event_list().size(),0);
    detection_cnt_fp = std::vector<int>(alg->get_detector_list().size(),0);
    event_cnt_fp = std::vector<int>(collector->get_event_list().size(),0);

    /********************************************************************
    ************************ setup short cuts ***************************
    *********************************************************************/
    QShortcut* pslh = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_Q), this, SLOT(sendLaserh()));
    QShortcut* psll = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_W), this, SLOT(sendLaserl()));
    QShortcut* pssl = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_Z), this, SLOT(stopLaser()));

    std::cout << "short cuts done "<<std::endl;
    /********************************************************************
    ************************ setup timer  *******************************
    *********************************************************************/
    collectTimer = new QTimer(this);
    connect(collectTimer, SIGNAL(timeout()), this, SLOT(collect()));
    collectTimer->setTimerType(Qt::PreciseTimer);
    collectTimer->start(50);

    refreshTimer = new QTimer(this);
    connect(refreshTimer, SIGNAL(timeout()), this, SLOT(refresh()));
    refreshTimer->setTimerType(Qt::PreciseTimer);
    refreshTimer->start(5);

    std::cout << "timer done "<<std::endl;

    /********************************************************************
    ************************ setup opto string  *************************
    *********************************************************************/

    int duration = pDo->get_opto_duration();
    int hp = pDo->get_opto_pulse_h();
    int lp = pDo->get_opto_pulse_l();
    int random_freq = pDo->get_opto_random_freq();
    int freq = 1000/(hp+lp);
    QString str1 = QString::number(duration) + " ms " + QString::number(freq) +" Hz";
    QString str2 = "(h:" + QString::number(hp) + "ms,l:" + QString::number(lp) + "ms)";
    ui->label_13->setText(str1);
    ui->label_9->setText(str2);

    ui->lineEdit_9->setText(QString::number(duration));
    ui->lineEdit_10->setText(QString::number(hp));
    ui->lineEdit_11->setText(QString::number(lp));
    ui->lineEdit_12->setText(QString::number(random_freq));

    return 0;
}

void PainDetectClient::on_trainButton_clicked()
{
    if (updateMode!=manual) return;
    // tobedone startTrainAcc();
    std::cout <<"train button clicked"<<std::endl;
    trainTriggered = false;
    acqUpdateCnt=0;
    ui->label_10->setText("Training");
    QPalette palette = ui->label_10->palette();
    palette.setColor(ui->label_10->foregroundRole(), Qt::red);
    ui->label_10->setPalette(palette);

    for (int i=0;i<threads.size();i++)
    {
        if (threads[i]->getType()==Training && !threads[i]->isRunning())
        {
            threads[i]->set_ui(ui);
            threads[i]->start(QThread::LowestPriority);
            training_cnt++;
        }
        else
            std::cout <<"training is going on: "<<threads[i]->get_detector()<<std::endl;
    }
    if (training_cnt==0)
    {
        ui->label_10->setText("Not Training");
        QPalette palette = ui->label_10->palette();
        palette.setColor(ui->label_10->foregroundRole(), Qt::black);
        ui->label_10->setPalette(palette);
    }
}

void PainDetectClient::on_baselineButton_Acc_clicked()
{
    detection_cnt_ready = false;
    ui->label_14->setText("Updating");
    QPalette palette = ui->label_14->palette();
    palette.setColor(ui->label_14->foregroundRole(), Qt::red);
    ui->label_14->setPalette(palette);

    std::cout << "update button clicked"<<std::endl;
    for(int j=0;j<alg->get_detector_list().size();j++)
    {
        if (alg->get_detector_list()[j]->get_type().compare("Single")==0)
        {
            if ((alg->get_detector_list()[j]->get_decoder_type().compare("PLDS")==0)
                    ||(alg->get_detector_list()[j]->get_decoder_type().compare("TLDS")==0) )
            {
                ModelDetector* pmd = static_cast<ModelDetector*>(alg->get_detector_list()[j]);
                pmd->update();
                std::cout << "update detector="<<alg->get_detector_list()[j]->get_name() <<std::endl;
                updateTableBaseline(j,pmd->get_meanBase(),pmd->get_stdBase());
            }
            if (alg->get_detector_list()[j]->get_decoder_type().compare("ModelFree")==0)
            {
                static_cast<ModelFreeDetector*>(alg->get_detector_list()[j])->update();
                std::cout << "update detector="<<alg->get_detector_list()[j]->get_name() <<std::endl;
            }
        }
    }
}

void PainDetectClient::updateTableBaseline(const int idx, const float mean_, const float std_)
{
    if (ui->tableWidget->rowCount()<idx)
    {
        std::cout << "invalid row idx="<<idx<<std::endl;
        return;
    }
    if (ui->tableWidget->item(idx, 5)==nullptr)
    {
        ui->tableWidget->setItem(idx, 5, new QTableWidgetItem(QString::number(mean_)));
        ui->tableWidget->setItem(idx, 6, new QTableWidgetItem(QString::number(std_)));
    }
    else
    {
        ui->tableWidget->item(idx, 5)->setText(QString::number(mean_));
        ui->tableWidget->item(idx, 6)->setText(QString::number(std_));
    }
}

void PainDetectClient::updateTableEvnDetCnt()
{
    for (int idx=0; idx<ui->tableWidget->rowCount();idx++)
    {
        if (ui->tableWidget->rowCount()<idx)
        {
            std::cout << "invalid row idx="<<idx<<std::endl;
            return;
        }

        //QString str = QString::number(detection_cnt[idx])+"/"+QString::number(event_cnt[0]);
        //QString str_fp = QString::number(detection_cnt_fp[idx])+"/"+QString::number(event_cnt_fp[0]);
        QString str = QString::number(detection_cnt[idx]);
        QString str_fp = QString::number(detection_cnt_fp[idx]);

        if (ui->tableWidget->item(idx, 2)==nullptr)
        {
            ui->tableWidget->setItem(idx, 2, new QTableWidgetItem(str));
            ui->tableWidget->setItem(idx, 3, new QTableWidgetItem(str_fp));
        }
        else
        {
            ui->tableWidget->item(idx, 2)->setText(str);
            ui->tableWidget->setItem(idx, 3, new QTableWidgetItem(str_fp));
        }
    }
}

void PainDetectClient::updateTableCCFthresh()
{
    int det_cnt = 0;
    for (auto iter = alg->get_detector_list().begin();iter<alg->get_detector_list().end();iter++)
    {
        std::string detectorType = (*iter)->get_type();
        if (detectorType.compare("CCF")==0)
        {
            CCFDetector* p_ccf = static_cast<CCFDetector*>(*iter);
            QString str = QString::number(p_ccf->get_ccf_thresh()[0])+","+QString::number(p_ccf->get_ccf_thresh()[1]);
            if (ui->tableWidget->item(det_cnt, 4)==nullptr)
            {
                ui->tableWidget->setItem(det_cnt, 4, new QTableWidgetItem(str));
            }
            else
            {
                ui->tableWidget->item(det_cnt, 4)->setText(str);
            }
            break;
        }
        det_cnt++;
    }
}


void PainDetectClient::on_pushButton_8_clicked()
{
    if (updateMode==auto_trial)
    {
        ui->label_43->setText("Manual Train");
        updateMode = manual;
        ui->pushButton_8->setText("Auto Train");
        QPalette palette = ui->label_43->palette();
        palette.setColor(ui->label_43->foregroundRole(), Qt::red);
        ui->label_43->setPalette(palette);
    }
    else
    {
        ui->label_43->setText("Auto Train");
        updateMode = auto_trial;
        ui->pushButton_8->setText("Manual Train");
        QPalette palette = ui->label_43->palette();
        palette.setColor(ui->label_43->foregroundRole(), Qt::green);
        ui->label_43->setPalette(palette);
    }
}

void PainDetectClient::on_pushButton_9_clicked()
{
    if (optoTriggerMode==auto_trial)
    {
        ui->label_42->setText("Random Opto");
        optoTriggerMode = random;
        ui->pushButton_9->setText("Manual Opto");
        QPalette palette = ui->label_42->palette();
        palette.setColor(ui->label_42->foregroundRole(), Qt::red);
        ui->label_42->setPalette(palette);
    }
    else if (optoTriggerMode==random)
    {
        ui->label_42->setText("Manual Opto");
        optoTriggerMode = manual;
        ui->pushButton_9->setText("Auto Opto");
        QPalette palette = ui->label_42->palette();
        palette.setColor(ui->label_42->foregroundRole(), Qt::green);
        ui->label_42->setPalette(palette);
    }
    else
    {
        ui->label_42->setText("Auto Opto");
        optoTriggerMode = auto_trial;
        ui->pushButton_9->setText("Random Opto");
        QPalette palette = ui->label_42->palette();
        palette.setColor(ui->label_42->foregroundRole(), Qt::green);
        ui->label_42->setPalette(palette);
    }
}

void PainDetectClient::handl_sc()
{
    std::cout << "short cut received"<<std::endl;
}

void PainDetectClient::on_pushButton_16_clicked()
{
    std::cout << "laser on received"<<std::endl;
    for (int i=0;i<threads.size();i++)
    {
        if (threads[i]->getType()==Laser && !threads[i]->isRunning())
        {
            std::vector<std::string> tmp;
            tmp.push_back("Stim");
            tmp.push_back("High");
            threads[i]->setParam(tmp);
            threads[i]->start(QThread::LowestPriority);
            std::cout << "laser on running"<<std::endl;
            break;
        }
    }
}

void PainDetectClient::on_pushButton_17_clicked()
{
    std::cout << "opto on received"<<std::endl;
    for (int i=0;i<threads.size();i++)
    {
        if (threads[i]->getType()==Opto && !threads[i]->isRunning())
        {
            threads[i]->start(QThread::LowestPriority);
            std::cout << "opto on running"<<std::endl;
            break;
        }
    }
}

void PainDetectClient::on_pushButton_18_clicked()
{
    std::cout << "trigger on received"<<std::endl;
    for (int i=0;i<threads.size();i++)
    {
        if (threads[i]->getType()==Trigger && !threads[i]->isRunning())
        {
            threads[i]->start(QThread::LowestPriority);
            std::cout << "trigger on running"<<std::endl;
            break;
        }
    }
}

void PainDetectClient::sendLaserh()
{
    std::cout << "laser on received"<<std::endl;
    ui->label_45->setText("H laser on");
    for (int i=0;i<threads.size();i++)
    {
        if (threads[i]->getType()==Laser && !threads[i]->isRunning())
        {
            std::vector<std::string> tmp;
            tmp.push_back("Stim");
            tmp.push_back("High");
            threads[i]->setParam(tmp);
            threads[i]->start(QThread::LowestPriority);
            std::cout << "laser on running"<<std::endl;
            break;
        }
    }
    //ui->label_45->setText("");
}
void PainDetectClient::sendLaserl()
{
    std::cout << "laser on received"<<std::endl;
    ui->label_45->setText("L laser on");
    for (int i=0;i<threads.size();i++)
    {
        if (threads[i]->getType()==Laser && !threads[i]->isRunning())
        {
            std::vector<std::string> tmp;
            tmp.push_back("Stim");
            tmp.push_back("Low");
            threads[i]->setParam(tmp);
            threads[i]->start(QThread::LowestPriority);
            std::cout << "laser on running"<<std::endl;
            break;
        }
    }
    //ui->label_45->setText("");
}

void PainDetectClient::stopLaser()
{
    std::cout << "stop laser on received"<<std::endl;
    pDo->stopLaser();
}

void PainDetectClient::sendDetection(std::string name)
{
    std::cout << "detection received "<<name<<std::endl;
    for (int i=0;i<threads.size();i++)
    {
        std::cout << threads[i]->getName()<<std::endl;
        if (threads[i]->getType()==Detection && !threads[i]->isRunning()
                && threads[i]->getName().compare(name)==0)
        {
            threads[i]->start(QThread::LowestPriority);
            std::cout <<name<< " running"<<std::endl;
            break;
        }
    }
}

void PainDetectClient::on_pushButton_19_clicked()
{
    sendDetection("detect1");
}

void PainDetectClient::on_pushButton_49_clicked()
{
    // update new params of the CCF
    QString alpha = ui->lineEdit_17->text();
    QString beta = ui->lineEdit_18->text();
    QString ffactor = ui->lineEdit_19->text();
    QString area_thresh = ui->lineEdit_20->text();

    for (auto iter = alg->get_detector_list().begin();iter<alg->get_detector_list().end();iter++)
    {
        std::string detectorType = (*iter)->get_type();
        if (detectorType.compare("CCF")==0)
        {
            CCFDetector* p_ccf = static_cast<CCFDetector*>(*iter);
            p_ccf->set_alpha(alpha.toFloat());
            p_ccf->set_beta(beta.toFloat());
            p_ccf->set_ffactor(ffactor.toFloat());
            p_ccf->set_area_thresh(area_thresh.toFloat());
            break;
        }
    }
}

// force ready
void PainDetectClient::on_pushButton_10_clicked()
{
    force_ready = !force_ready;
    if (force_ready)
        ui->label_5->setText("Force");
    else
        ui->label_5->setText("Normal");
}

void PainDetectClient::sendOpto()
{
    std::cout << "opto on received"<<std::endl;
    for (int i=0;i<threads.size();i++)
    {
        if (threads[i]->getType()==Opto && !threads[i]->isRunning())
        {
            threads[i]->start(QThread::LowestPriority);
            std::cout << "opto on running"<<std::endl;
            break;
        }
    }
}

void PainDetectClient::on_pushButton_11_clicked()
{
    sendOpto();
}

void PainDetectClient::on_pushButton_2_clicked()
{
    std::cout << "detection started!"<<std::endl;
    detection_cnt_ready = true;
    ui->label_14->setText("Detecting");
    QPalette palette = ui->label_14->palette();
    palette.setColor(ui->label_14->foregroundRole(), Qt::green);
    ui->label_14->setPalette(palette);
}

void PainDetectClient::on_pushButton_3_clicked()
{
    int duration = (ui->lineEdit_9->text()).toInt();
    int oh = (ui->lineEdit_10->text()).toInt();
    int ol = (ui->lineEdit_11->text()).toInt();
    int rf = (ui->lineEdit_12->text()).toInt();
    pDo->set_opto_pulse_h(oh);
    pDo->set_opto_pulse_l(ol);
    pDo->set_opto_duration(duration);
    pDo->set_opto_random_freq(rf);
    int freq = 1000/(oh+ol);
    QString str1 = QString::number(duration) + " ms " + QString::number(freq) +" Hz";
    QString str2 = "(h:" + QString::number(oh) + "ms,l:" + QString::number(ol) + "ms)";
    ui->label_13->setText(str1);
    ui->label_9->setText(str2);
    std::cout << "opto params updated"<<std::endl;
    random_opto_freq = rf;
}

void PainDetectClient::on_pushButton_5_clicked()
{
    for (auto iter = alg->get_detector_list().begin();iter<alg->get_detector_list().end();iter++)
    {
        int det_idx = iter-alg->get_detector_list().begin();
        std::string detectorType = (*iter)->get_type();
        // draw single region deocding plots
        if (detectorType.compare("Single")==0)
        {
            std::string decoderType = (*iter)->get_decoder_type();
            if (decoderType.compare("PLDS")==0 || decoderType.compare("TLDS")==0)
            {
                ModelDetector* pdc = static_cast<ModelDetector*>(*iter);
                std::cout << "n_decoder="<<pdc->n_trained_decoders()<<std::endl;
                pdc->use_last_model();
            }
        }
    }
}

void PainDetectClient::on_pushButton_clicked()
{
    int iteration = (ui->lineEdit_2->text()).toInt();
    int time = (ui->lineEdit_3->text()).toInt();
    int train_length = (ui->lineEdit_4->text()).toInt();

    for (auto iter = alg->get_detector_list().begin();iter<alg->get_detector_list().end();iter++)
    {
        std::string decoderType = (*iter)->get_decoder_type();

        if (decoderType.compare("PLDS")==0 || decoderType.compare("TLDS")==0)
        {
            ModelDetector* pdc = static_cast<ModelDetector*>(*iter);
            pdc->set_l_train(train_length);
        }
    }
}
