#ifndef PAINDETECTCLIENT_H
#define PAINDETECTCLIENT_H

#include <QMainWindow>
#include "qcustomplot.h"
#include <vector>
#include "plexDoManager.h"
#include "algorithms.h"
#include "configManager.h"
#include "dataCollector.h"
#include "qthreadmanager.h"

namespace Ui {
class PainDetectClient;
}
using namespace painBMI;

enum TESTMODE{Continuous,Trial};

class PainDetectClient : public QMainWindow
{
    Q_OBJECT

public:
    explicit PainDetectClient(QWidget *parent = 0);
    ~PainDetectClient();

    //construct detector/decoder objects based on configuration
    void init(std::string filename);
    void closeEvent (QCloseEvent *event);

    //TODO: other gets and sets
private slots:
    int makePlot();
    void collect();
    void refresh();
    void handl_sc();
    // short cuts
    void sendLaserh();
    void sendLaserl();
    void stopLaser();
    void sendOpto();
    void sendDetection(std::string name);

    void on_trainButton_clicked();

    void on_baselineButton_Acc_clicked();

    void on_pushButton_8_clicked();

    void on_pushButton_9_clicked();

    void on_pushButton_16_clicked();

    void on_pushButton_17_clicked();

    void on_pushButton_18_clicked();

    void on_pushButton_19_clicked();

    void on_pushButton_49_clicked();

    void on_pushButton_10_clicked();

    void on_pushButton_11_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_pushButton_5_clicked();

    void on_pushButton_clicked();

private:
    Ui::PainDetectClient *ui;
    // data collector pointer
    PlexDataCollector* collector;
    // algorithms pointer
    AlgorithmManager* alg;
    // DO pointer
    PlexDo* pDo;
    // ui & plot
    //raster plot objects
    QCPAxisRect* rasterRect;
    QCPColorMap* raster;
    std::vector<QCPItemText*> rasterTextLabels;
    QCPItemLine* rasterLine;
    QCPColorScale *rasterColorScale;

    //cbar objects
    QCPItemLine* cbarLine;
    QCPBars *Cbar;
    QVector<double> cbarData;
    QVector<double> cbarx;

    //z-plot objects
    std::vector<QCPItemText*> zplotTextLabels;
    std::vector<QCPItemLine*> zplotThreshLines;
    std::vector<QCPItemLine*> detection_lines;
    std::vector<QCPItemLine*> stim_event_lines;
    std::vector<int> detection_cnt;
    std::vector<int> event_cnt;
    std::vector<int> detection_cnt_fp;
    std::vector<int> event_cnt_fp;

    // timer
    QTimer* collectTimer;
    QTimer* refreshTimer;
    //QTimer* triggerTimer;

    int plotTime; // The time length that will be shown on the plot, in sec
    int n_conf;// total number of confident intervals that will be shown.

    // **to be done, may be modified
    UPDATE_MODE updateMode;
    UPDATE_MODE optoTriggerMode;
    int random_opto_freq;
    TESTMODE testMode;
    bool trialStarted;
    bool ctl_trialStarted;

    int n_detectors;// total number of detectors
    bool no_detection;
    bool force_ready;
    bool detection_cnt_ready;

    //detection falls in trial_detect_time after stimulus will be counted as TP/FP
    float trial_detect_time;

    // config
    ConfigManager conf;
    // threads
    std::vector<ThreadManager*> threads;

    int ccf_thr_idx;
    int ccf_det_idx;
    int emg_thr_idx;
    int emg_det_idx;

    // methods
    void start();
    void destroy();
    int addTableRow(QString type, QString region);
    void updateTableBaseline(const int idx, const float mean_, const float std_);
    void updateTableEvnDetCnt();
    // for ccf
    void updateTableCCFthresh();
    // for greedy
    int greedy_cnt;
    // for training bins count: traning start after triggered acqUpdateCnt bins later.
    int acqUpdateCnt;
    bool trainTriggered;
    // detectections before trial_detect_bins bins after stimulation, is counted as TP.
    int trial_detect_bins;
    int trialDetectBinCnt;
    bool isLastStimPain;
    // for single region plots
    int plot_cnt;
    int cnf_cnt;
    int training_cnt;
    float c_scaling_factor;
    // for timing
    double decoding_time_s;
    std::vector<QCustomPlot*> zplots;
    // refresh methods
    void updateRasterPlot(double key,string_vec regions,Eigen::VectorXf spikes,int region_idx,int unit_idx_shift);
    void processEvents(double key,int region_idx);
    void processHlaser(double key,int region_idx);
    void processLlaser(double key,int region_idx);
    void processEMG(EMGDetector* p_emg, double key);
    void processCCF(CCFDetector* p_ccf, double key,int det_idx);
    void updateDetectionCnt(int det_idx);
    void processGreedy(GreedyDetector* p_greedy,double key,int det_idx,std::vector<Eigen::VectorXf>& spike_vec, string_vec regions);
    void processMF(ModelFreeDetector* p_mf,double key,int det_idx,Eigen::VectorXf &spikes, string_vec regions, int region_idx);
    void processModel(ModelDetector* pdc, double key,int det_idx,Eigen::VectorXf &spikes, int region_idx,int unit_idx_shift);
    void processSingle(DetectorBase* pd, double key,int det_idx,Eigen::VectorXf &spikes, int region_idx,int unit_idx_shift, string_vec regions);
    void processDetectors(double key, std::vector<Eigen::VectorXf>& spike_vec, Eigen::VectorXf &tmpSpike, string_vec &regions,int region_idx,int unit_idx_shift);
    void processFlags();
    void updateTrialNumbers();
};

#endif // PAINDETECTCLIENT_H
