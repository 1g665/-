#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QRandomGenerator>
#include<QList>
#include<cmath>
#include <QMediaPlayer>
#include <QAudioOutput>
struct Riddle {
    QString question;
    QString answer;
};
class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    // 按钮点击后执行的动作函数
    void onStartGame();
    void onPlayerGuess();
    void aiAction(QString currentLog);

private:
    // --- 这里的变量必须声明，否则 .cpp 里会因为“找不到变量”而报红圈 ---
    QWidget *gameContainer;
    QLabel *statusLabel;
    QLabel *logLabel;
    QLineEdit *playerSecretInput;
    QLineEdit *playerGuessInput;
    QPushButton *startBtn;
    QPushButton *guessBtn;
    QPushButton *rescueBtn; // ✨ 新增：拯救按钮
    QPushButton *reviewBtn;
    QList<Riddle> riddleBank;
    // 在 widget.h 的 private 区域添加
    // 在 widget.h 的 private 区域添加 (47行往后)
    QLabel *bgLayer;         // 原有的静态背景 (downn.png)
    QLabel *movieLayer;      // 动图背景层 (download.gif)
    QMovie *bgMovie;         // 动图对象
    QLabel *storyText;       // 剧情显示标签
    QTimer *typeTimer;       // 打字机定时器
    QString fullStoryText;   // 完整的剧情文本
    int charIndex = 0;       // 打字索引
    QPushButton *enterBtn;   // “进入系统”按钮
    QLabel *flashScreenColor;   // 补上这个
    QLabel *staticText;         // 补上这个
    QPushButton *btnLayer;
    QLabel *dangerBgLayer; // 专门用于放置第九轮动图的层
    QMovie *dangerMovie;   // 存储你的动图
    QMovie *failMovie; // 用于播放失败动图
    QLabel *storyLabel; // 用于在屏幕上显示剧情文字
    QTimer *roundTimer;       // 控制倒计时的定时器
    QLabel *countdownLabel;   // 显示时间的 UI 标签
    int timeLeft;             // 剩余秒数
    void handleTimeout();     // 处理超时的专属函数
    QMediaPlayer *winPlayer;
    QAudioOutput *winAudioOutput;
    // 封面背景音乐
    QMediaPlayer *firstPlayer;
    QAudioOutput *firstAudioOutput;
    // 猜谜过程音乐
    QMediaPlayer *guessPlayer;
    QAudioOutput *guessAudioOutput;
    // 新增的三大音乐播放器
    QMediaPlayer *playPlayer;
    QAudioOutput *playAudioOutput;

    QMediaPlayer *failMusicPlayer;
    QAudioOutput *failAudioOutput;

    QMediaPlayer *reviewPlayer;
    QAudioOutput *reviewAudioOutput;
    // 数字炸弹血条（进度条）
    QProgressBar *playerBombFuse;
    QProgressBar *aiBombFuse;
QLabel *explosionLabel;
    // 游戏核心数据
    QString playerSecret;
    QString aiSecret;
    int roundCount;
    bool gameRunning;
    QStringList aiPool; // AI 用来排除数字的“池子”

    // 逻辑判定函数
    int checkMatch(QString secret, QString guess);
};
#endif // WIDGET_H