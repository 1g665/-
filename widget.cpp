#include "widget.h"
#include <QRandomGenerator>
#include <QMessageBox>
#include <QScrollArea>
#include <QLabel>
#include <QTimer>
#include<QInputDialog>
#include <QMovie>
#include <QPushButton>

// =========================================================================
// 1. 初始化房间 (包含了封面 + 游戏界面搭建)
// =========================================================================
Widget::Widget(QWidget *parent) : QWidget(parent), gameRunning(false) {
       explosionLabel = nullptr;
    rescueBtn = nullptr;
       failMovie = nullptr;       // ✨ 必须补上这一行！扼杀野指针！
    this->setWindowTitle("达拉崩吧：零日拦截");
    this->setFixedSize(1000, 600);
    // ========== 胜利音乐播放器初始化 ==========
    winPlayer = new QMediaPlayer(this);
    winAudioOutput = new QAudioOutput(this);
    winPlayer->setAudioOutput(winAudioOutput);

    // 设置音量，0.8代表80%的音量
    winAudioOutput->setVolume(0.3);

    // 自动获取 math.exe 所在的路径，并拼接上你刚才建的 music 文件夹
    QString musicPath = QCoreApplication::applicationDirPath() + "/music/winmusic.mp3";
    winPlayer->setSource(QUrl::fromLocalFile(musicPath));
    // ==========================================
    // ========== 封面背景音乐初始化 ==========
    firstPlayer = new QMediaPlayer(this);
    firstAudioOutput = new QAudioOutput(this);
    firstPlayer->setAudioOutput(firstAudioOutput);

    firstAudioOutput->setVolume(0.3); // 背景音建议稍微小一点，0.5比较合适

    // 设置循环播放（这样音乐播完会自动重头开始）
    firstPlayer->setLoops(QMediaPlayer::Infinite);

    // 路径指向你的 first.mp3（记得确认文件后缀，如果是.wav就改成.wav）
    QString firstPath = QCoreApplication::applicationDirPath() + "/music/first.mp3";
    firstPlayer->setSource(QUrl::fromLocalFile(firstPath));

    // 重点：初始化完直接开播！
    firstPlayer->play();
    // ========================================
    // ========== 猜谜过程音乐初始化 ==========
    guessPlayer = new QMediaPlayer(this);
    guessAudioOutput = new QAudioOutput(this);
    guessPlayer->setAudioOutput(guessAudioOutput);

    guessAudioOutput->setVolume(0.3); // 建议音量适中
    guessPlayer->setLoops(QMediaPlayer::Infinite); // 猜的时候一直循环

    QString guessPath = QCoreApplication::applicationDirPath() + "/music/guess.mp3";
    guessPlayer->setSource(QUrl::fromLocalFile(guessPath));
    // ========== 常规游戏背景音 (play) ==========
    playPlayer = new QMediaPlayer(this);
    playAudioOutput = new QAudioOutput(this);
    playPlayer->setAudioOutput(playAudioOutput);
    playAudioOutput->setVolume(0.3);
    playPlayer->setLoops(QMediaPlayer::Infinite); // 循环播放
    playPlayer->setSource(QUrl::fromLocalFile(QCoreApplication::applicationDirPath() + "/music/play.mp3"));

    // ========== 失败结局音乐 (fail) ==========
    failMusicPlayer = new QMediaPlayer(this);
    failAudioOutput = new QAudioOutput(this);
    failMusicPlayer->setAudioOutput(failAudioOutput);
    failAudioOutput->setVolume(0.5); // 失败音乐稍微大点，渲染悲剧感
    failMusicPlayer->setSource(QUrl::fromLocalFile(QCoreApplication::applicationDirPath() + "/music/fail.mp3"));

    // ========== 复盘音乐 (review) ==========
    reviewPlayer = new QMediaPlayer(this);
    reviewAudioOutput = new QAudioOutput(this);
    reviewPlayer->setAudioOutput(reviewAudioOutput);
    reviewAudioOutput->setVolume(0.3); // 复盘需要安静思考
    reviewPlayer->setLoops(QMediaPlayer::Infinite);
    reviewPlayer->setSource(QUrl::fromLocalFile(QCoreApplication::applicationDirPath() + "/music/review.aac"));
    // 注意：这里先不要写 .play()，因为现在还在封面呢
    // --- 封面系统 ---
    // --- 1. 初始化原有的静态背景 (第二页) ---
    bgLayer = new QLabel(this);
    bgLayer->setPixmap(QPixmap(":/downn.png"));
    bgLayer->setScaledContents(true);
    bgLayer->setGeometry(0, 0, 1000, 600);


    // 在 bgLayer->lower(); 之后加上这一行
    // 这会给背景蒙上一层浅浅的黑色滤镜，让前面的亮粉色/绿色文字更跳出来
    bgLayer->setStyleSheet("background-color: rgba(0, 0, 0, 80);");
    // --- 1.5 预加载第 9 轮危机背景 (初始隐藏) ---
    dangerBgLayer = new QLabel(this);
    dangerBgLayer->setGeometry(0, 0, 1000, 600);
    dangerBgLayer->setScaledContents(true);
    dangerBgLayer->hide(); // 👈 关键：初始必须隐藏

    dangerMovie = new QMovie(":/nine.gif"); // 👈 确保资源名正确
    dangerBgLayer->setMovie(dangerMovie);

    // 把它放到最底层，这样它就不会挡住第一页的剧情和第二页的磁带墙
    dangerBgLayer->lower();
    // --- 2. 初始化动图层 (第一页) ---
    movieLayer = new QLabel(this);
    bgMovie = new QMovie(":/download.gif"); // 先只传路径
    bgMovie->setParent(this);               // 手动设父对象，防止报错
    movieLayer->setMovie(bgMovie);
    bgMovie->start();
    movieLayer->setScaledContents(true);
    movieLayer->setGeometry(0, 0, 1000, 600);
    movieLayer->raise(); // 盖在最上面

    // --- 3. 打字机 UI 设置 ---
    // --- 3. 打字机 UI 设置 ---
    storyText = new QLabel(movieLayer);
    storyText->setGeometry(280, 80, 520, 420);
    storyText->setWordWrap(true);
    storyText->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    storyText->setStyleSheet(
        "color: #00FF41; "
        "font-size: 17px; "
        "font-weight: bold; "
        "font-family: 'Consolas', 'Monospace'; "
        "background: rgba(0, 0, 0, 200); "
        "border: 2px solid #FF3232; "
        "border-radius: 12px; "
        "padding: 20px;"
        );

    fullStoryText = "【 🚨 逃生警告：坐标已被锁定 🚨 】\n\n"
                    "勇士找到了公主，但恶龙在公主身上安装了「位置追踪炸弹」！\n"
                    "你必须在 10 轮内完成解码，否则炸弹将自动引爆。\n\n"
                    "警告：每一轮只有 30 秒破译时间！\n"  // ✨ 新增这一行
                    "⚡ 破译协议：\n"
                    "1. 输入 4 位数字，只有【数值与位置完全匹配】才能解除锁定。\n"
                    "2. 匹配度达到 4 时，炸弹方可安全拆除。\n"
                    "3. ⚠️ 致命威胁：巨龙正通过你输入的信号【反向追踪】你们的位置！\n"
                    "4. 若巨龙先破译出你的数字，它将锁定坐标并瞬间降临，引爆炸弹。\n\n"
                    "任务：带上最强大脑，在巨龙降临前，切断信号，带公主回家！";

    typeTimer = new QTimer(this);
    connect(typeTimer, &QTimer::timeout, this, [=](){
        if(charIndex < fullStoryText.length()){
            storyText->setText(fullStoryText.left(charIndex + 1));
            charIndex++;
        }else{
            typeTimer->stop();
        }
    });
    typeTimer->start(40);
    // --- 4. 进入按钮 ---
    enterBtn = new QPushButton("进入系统", this);
    enterBtn->setGeometry(400, 480, 200, 50);
    enterBtn->setStyleSheet(
        "QPushButton {"
        "   background: rgba(0, 255, 65, 40);"    // 半透明极客绿
        "   color: #00FF41;"                      // 字体纯绿色
        "   border: 2px solid #00FF41;"           // 绿色边框
        "   border-radius: 10px;"
        "   font-size: 20px; font-weight: bold;"
        "}"
        "QPushButton:hover {"                     // 鼠标放上去会发光
        "   background: rgba(0, 255, 65, 100);"
        "}"
        );
    enterBtn->raise();
   flashScreenColor = new QLabel(this);
    // 23行：微调位置和大小 (让它往右下缩一点，避开边框)
    flashScreenColor->setGeometry(370, 160, 270, 209);
    // 24行：加入 border-radius (圆角) 和 border (外发光边缘)
    flashScreenColor->setStyleSheet(
        "background-color: rgba(255, 0, 0, 60);"
        "border-radius: 30px;" // 👈 灵魂：数值越大角越圆
        "border: 2px solid rgba(255, 50, 50, 100);" // 👈 模拟屏幕漏光边缘
        );
    flashScreenColor->hide();
    staticText = new QLabel(this);
    staticText->setPixmap(QPixmap(":/you.png"));
    staticText->setScaledContents(true);
    staticText->setGeometry(375, 220, 250, 100);
    staticText->raise();


    btnLayer = new QPushButton(this);
    btnLayer->setGeometry(345, 155, 310, 230);
    btnLayer->setStyleSheet("background: transparent; border: none;");
    btnLayer->raise();
    btnLayer->hide();
    connect(enterBtn, &QPushButton::clicked, [=](){
        movieLayer->hide();   // 隐藏剧情动图
        enterBtn->hide();     // 隐藏进入按钮
        storyText->hide();    // 隐藏剧情文字

        // --- 唤醒原首页元素 ---
        flashScreenColor->show(); // 显示红色闪烁框
        staticText->show();       // 显示游戏标题 (you.png)
        btnLayer->show();
});        // 显示那个透明的 Game Start 按钮

    // --- 游戏容器与赛博皮肤 ---
    gameContainer = new QWidget(this);
    gameContainer->setGeometry(0, 0, 1000, 600);
    gameContainer->hide(); // 先藏起来，等点击再显示
    gameContainer->setStyleSheet(
        "QWidget { background-color: rgba(5, 5, 5, 150); color: #FF00FF; font-family: 'Consolas'; }"
    // 2. 输入框：降低边框亮度，使用半透明背景
    "QLineEdit { "
    "border: 1px solid rgba(112, 255, 255, 150); "
    "background: rgba(0, 0, 0, 100); "
    "color: #70FFFF; "
        "   font-size: 50px; "                   // 👈 1. 进一步加大字号（可以根据需要调到 60px）
        "   font-weight: bold; "                 // 👈 2. 字体加粗
        "   qproperty-alignment: 'AlignCenter'; "// 👈 3. 关键：让输入的数字水平居中
        "   font-family: 'Consolas';"
    "}"

    // 3. 按钮：使用虚线或细边框，鼠标悬停时才发光
    "QPushButton { "
    "border: 1px solid #70FFFF; "
    "background: rgba(0, 255, 255, 20); "
    "color: #70FFFF; "
    "padding: 5px; "
    "}"
    "QPushButton:hover { background: rgba(0, 255, 255, 60); }"

    // 4. 进度条：外框调细，颜色变淡
    "QProgressBar { "
    "border: 1px solid rgba(112, 255, 255, 100); "
    "background: rgba(0, 0, 0, 150); "
    "height: 8px; "
    "text-align: center; "
    "}"
    "QProgressBar::chunk { background-color: #00CCCC; }"
);
    // --- ⏳ 倒计时 UI 设置 ---
    countdownLabel = new QLabel("00:30", gameContainer); // 依附在游戏界面上
    countdownLabel->setGeometry(820, 20, 150, 50);       // 放在右上角
    countdownLabel->setStyleSheet("color: #00FF41; font-size: 36px; font-weight: bold; font-family: 'Consolas'; background: transparent;");
    countdownLabel->setAlignment(Qt::AlignCenter);
    countdownLabel->hide();

    // --- ⏳ 倒计时逻辑 ---
    roundTimer = new QTimer(this);
    connect(roundTimer, &QTimer::timeout, this, [=](){
        timeLeft--;
        countdownLabel->setText(QString("00:%1").arg(timeLeft, 2, 10, QChar('0')));

        // 剩 5 秒时：字体变大，颜色变血红，背景闪烁！
        if (timeLeft <= 5) {
            if (timeLeft % 2 == 0) {
                countdownLabel->setStyleSheet("color: #FF0000; font-size: 42px; font-weight: bold; background: rgba(255,0,0,60); border-radius: 10px;");
            } else {
                countdownLabel->setStyleSheet("color: #FF3030; font-size: 36px; font-weight: bold; background: transparent;");
            }
        }

        // 时间耗尽
        if (timeLeft <= 0) {
            roundTimer->stop();
            handleTimeout(); // 触发超时惩罚！
        }
    });
    // --- 内部布局 (双头像 + 血条) ---
    QVBoxLayout *mainLayout = new QVBoxLayout(gameContainer);
    QHBoxLayout *headerLayout = new QHBoxLayout();

    // 玩家区
    QVBoxLayout *pBox = new QVBoxLayout();
    QLabel *pAvatar = new QLabel();
    pAvatar->setPixmap(QPixmap(":/woman.png").scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    pAvatar->setStyleSheet("border: 2px solid #FF3232; background-color: rgba(255, 0, 0, 20);");
    playerBombFuse = new QProgressBar();
    playerBombFuse->setRange(0, 4);
    playerBombFuse->setValue(0);
    pBox->addWidget(new QLabel("<b>[ 达拉崩吧 ]</b>"));
    pBox->addWidget(pAvatar);
    pBox->addWidget(new QLabel("定位进度:"));
    pBox->addWidget(playerBombFuse);

    // AI 区
    QVBoxLayout *aBox = new QVBoxLayout();
    QLabel *aAvatar = new QLabel();
    aAvatar->setPixmap(QPixmap(":/ss.png").scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    aAvatar->setStyleSheet("border: 2px solid #FF3232; background-color: rgba(255, 0, 0, 20);");
    aiBombFuse = new QProgressBar();
    aiBombFuse->setRange(0, 4);
    aiBombFuse->setValue(0);
    aBox->addWidget(new QLabel("<b>[ 昆图库塔 ]</b>"));
    aBox->addWidget(aAvatar);
    aBox->addWidget(new QLabel("破解深度:"));
    aBox->addWidget(aiBombFuse);

    headerLayout->addLayout(pBox);
    headerLayout->addStretch();
    headerLayout->addLayout(aBox);
    mainLayout->addLayout(headerLayout);

    // --- 交互组件 ---
    statusLabel = new QLabel("⚠️ 紧急物理阻断"   "【警告】巨龙正在同步你的位置！\n请输入 4 位反制代码以强行切断追踪信号：");
                             playerSecretInput = new QLineEdit();
                      playerSecretInput->setFixedHeight(60); // ✨ 插入到这里
    startBtn = new QPushButton(">>🚨 启动紧急反制");

    playerGuessInput = new QLineEdit();
    playerGuessInput->setFixedHeight(60); // ✨ 插入到这里
    playerGuessInput->setPlaceholderText(">> 输入拦截的信号数据...");
    playerGuessInput->setEnabled(false);
    guessBtn = new QPushButton(">>🔑 尝试解密");
    guessBtn->setEnabled(false);

    logLabel = new QLabel("[ WAITING FOR CONNECTION... ]");
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidget(logLabel);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("background: #000; border: 1px solid #005520;");

    mainLayout->addWidget(statusLabel);
    mainLayout->addWidget(playerSecretInput);
    mainLayout->addWidget(startBtn);
    mainLayout->addWidget(new QLabel("------------------------------------------"));
    mainLayout->addWidget(playerGuessInput);
    mainLayout->addWidget(guessBtn);
    mainLayout->addWidget(scrollArea);

    // --- 信号连接 ---
    // --- 信号连接：自毁警报模式 ---
    // --- 信号连接：改为橙红交替的“自毁脉冲”模式 ---
    QTimer *timer = new QTimer(this);
    // 128行保持不变
    // 从129行开始替换
    connect(timer, &QTimer::timeout, this, [=](){
        // ✨ 新增逻辑：如果爆炸/胜利层显示了，就让它闪烁
       if (explosionLabel && explosionLabel->isVisible()) {
            static int expFrame = 0;
            expFrame++;
            // 这种样式会让图片边缘带上红光，并快速闪烁背景
            if (expFrame % 2 == 0) {
                explosionLabel->setStyleSheet("border: 2px solid red; background-color: rgba(255, 0, 0, 40);");
            } else {
                explosionLabel->setStyleSheet("border: 2px solid white; background-color: rgba(255, 255, 255, 30);");
            }
            return; // 💥 重要：一旦处于终结画面，就不执行下面的首页闪烁了
        }

       // --- 下面是你原来的首页闪烁逻辑 ---
       if (enterBtn->isHidden()) {
           static int frame = 0;
           frame++;

           // ✨ 新增：定义头像框的亮红和暗红样式
           QString avatarBright = "border: 2px solid #FF0000; background-color: rgba(255, 0, 0, 40);";
           QString avatarDim = "border: 2px solid #8B0000; background-color: rgba(255, 0, 0, 10);";

           QString screenBaseStyle = "border-radius: 20px; border: 1px solid rgba(255, 50, 50, 80);";

           if (frame % 2 == 0) {
               // 原有逻辑：标题闪烁
               flashScreenColor->setStyleSheet(screenBaseStyle + "background-color: rgba(255, 0, 0, 70);");
               staticText->setVisible(true);

               // ✨ 新增：头像框变亮
               pAvatar->setStyleSheet(avatarBright);
               aAvatar->setStyleSheet(avatarBright);
           } else {
               // 原有逻辑
               flashScreenColor->setStyleSheet(screenBaseStyle + "background-color: rgba(255, 120, 0, 40);");
               staticText->setVisible(false);

               // ✨ 新增：头像框变暗
               pAvatar->setStyleSheet(avatarDim);
               aAvatar->setStyleSheet(avatarDim);
           }
       }
        } );// ✨ 这个反括号刚好关上 if

    // 直到145行结束
    // 别忘了把时间调得舒服一点，建议 150ms
    timer->start(150);

    connect(startBtn, &QPushButton::clicked, this, &Widget::onStartGame);
    connect(guessBtn, &QPushButton::clicked, this, &Widget::onPlayerGuess);

    connect(btnLayer, &QPushButton::clicked, [=](){
        timer->stop();
        if (firstPlayer) {
            firstPlayer->stop();
        }
if (playPlayer) playPlayer->play();
        // ✨ 在这里（第 254 行之后）插入：
        bgLayer->setPixmap(QPixmap(":/back.gif")); // 将背景从磁带墙切换为复古电脑[span_0](end_span)
        bgLayer->setStyleSheet("background-color: rgba(0, 0, 0, 80);"); // 增加遮罩保证文字清晰[span_1](end_span)

            // ⚠️ 删掉或注释掉这一行（第 254 行），不要隐藏它：
            // bgLayer->hide();
        bgLayer->show();

            flashScreenColor->hide();
        staticText->hide();
        btnLayer->hide();
        gameContainer->show();
    });

        // 这里只负责“造出”这个标签，不负责判断输赢
        explosionLabel = new QLabel(this);
        explosionLabel->setGeometry(0, 0, 1000, 600);
        explosionLabel->setScaledContents(true);
        explosionLabel->hide(); // 先藏好
        explosionLabel->raise();
        // ... 在你之前的 explosionLabel 初始化代码下面插入 ...
        rescueBtn = new QPushButton(">>🚨 尝试最后拦截<<", this);
        rescueBtn->setGeometry(400, 450, 200, 50); // 放在屏幕中下方
        rescueBtn->setStyleSheet(
            "QPushButton { "
            "background-color: #000; border: 2px solid #ff0041; color: #ff0041; "
            "font-weight: bold; font-size: 16px; border-radius: 10px; "
            "}"
            "QPushButton:hover { background-color: #ff0041; color: #000; }"
            );
        rescueBtn->hide(); // 初始必须藏起来
        rescueBtn->raise(); // 确保它在最顶层        // 放在最顶层
        // --- 📊 初始化复盘按钮（新加入） ---

                storyLabel = new QLabel(this);
        // 设置在屏幕中央区域，高度给足，方便多行文字显示
        storyLabel->setGeometry(100, 100, 800, 350);
        // 样式：红色高亮文字，加一点点半透明底色，确保在粉色蘑菇云动图上也能看清
        storyLabel->setStyleSheet("color: #FF3030; "
                                  "font-family: 'Microsoft YaHei'; "
                                  "font-size: 26px; "
                                  "font-weight: bold; "
                                  "background-color: rgba(0, 0, 0, 80); " // 稍微深一点的遮罩
                                  "border-radius: 20px; "
                                  "padding: 20px;");
        storyLabel->setAlignment(Qt::AlignCenter);
        storyLabel->setWordWrap(true);
        storyLabel->hide(); // 初始状态保持隐藏
        storyLabel->raise(); // 必须 raise，否则会被背景动图挡住

        reviewBtn = new QPushButton("📊 查看对决全程", this);
        reviewBtn->setGeometry(400, 520, 200, 50); // 放在救援按钮 (y=450) 下方
        reviewBtn->setStyleSheet(
            "QPushButton {"
            "  background-color: #000; border: 2px solid #00ff00; color: #00ff00;"
            "  font-weight: bold; font-size: 16px; border-radius: 10px;"
            "}"
            "QPushButton:hover { background-color: #00ff00; color: #000; }"
            );
        reviewBtn->hide();
        reviewBtn->raise();

        connect(reviewBtn, &QPushButton::clicked, [=](){
            if (winPlayer) winPlayer->stop();
            if (failMusicPlayer) failMusicPlayer->stop();
            if (playPlayer) playPlayer->stop();
            if (guessPlayer) guessPlayer->stop();
            if (reviewPlayer) reviewPlayer->play();
            // ✨ 2. 播放专门的复盘音乐

            // 1. 让霸屏的照片隐身，露出底层的日志
            if (explosionLabel) {
                explosionLabel->hide();
            }

            // 2. 高亮左侧日志区，方便查看
            logLabel->setStyleSheet(
                "border: 2px solid #00ff00; "
                "background-color: rgba(0, 0, 0, 200); "
                "color: #00ff00; "
                "font-family: 'Consolas';"
                );

            // 3. 提示进入档案室模式
            QMessageBox::information(this, "作战复盘", "战果图已收起。请在主界面左侧滑动查看完整的对战逻辑与数据过滤过程。");
        });

    // 填充题库：你想加多少就加多少
    // 显式调用 Riddle 构造方式
    riddleBank.append(Riddle{"8的一半是什么？", "0"});
    riddleBank.append(Riddle{"1-10什么数字最喜欢喝酒？", "1"});
    riddleBank.append(Riddle{"1-10哪个数字最痛？", "5"});
    riddleBank.append(Riddle{"1-10什么数字最安静？", "2"});
    riddleBank.append(Riddle{"4在郊区的房子租给谁了？", "5"});
    if (staticText) {
        staticText->hide();
        staticText->setVisible(false);
        staticText->setAttribute(Qt::WA_TransparentForMouseEvents);
    }
} //

Widget::~Widget() {}

// =========================================================================
// 2. 游戏核心逻辑
// =========================================================================

void Widget::onStartGame() {
    playerSecret = playerSecretInput->text();
    if (playerSecret.length() != 4) {
        QMessageBox::warning(this, "警告", "请输入 4 位数字！");
        return;
    }

    // AI 生成秘密
    aiSecret = "";
    for(int i=0; i<4; i++) aiSecret += QString::number(QRandomGenerator::global()->bounded(10));

    // 初始化 AI 排除池
    aiPool.clear();
    for (int i = 0; i <= 9999; i++) aiPool.push_back(QString("%1").arg(i, 4, 10, QChar('0')));

    roundCount = 0;
    gameRunning = true;
    playerBombFuse->setValue(0);
    aiBombFuse->setValue(0);

    // 切换 UI 状态
    playerSecretInput->setEnabled(false);
    startBtn->setEnabled(false);
    playerGuessInput->setEnabled(true);
    guessBtn->setEnabled(true);

    statusLabel->setText("<font color='#00ff41'>对决开始！双方已进入潜伏状态。</font>");
    logLabel->setText("系统提示：请开始你的第一轮猜测。<br>");// 重置并启动倒计时
    countdownLabel->show();
    timeLeft = 30;
    countdownLabel->setText("00:30");
    countdownLabel->setStyleSheet("color: #00FF41; font-size: 36px; font-weight: bold; background: transparent;");
    roundTimer->start(1000); // 1000毫秒 = 1秒触发一次

} // 👈 这是 onStartGame 函数的结束大括




void Widget::onPlayerGuess() {
    QString pGuess = playerGuessInput->text();
    if (pGuess.length() != 4) return;
roundTimer->stop(); // 👈 关键：玩家成功提交，计时器立刻暂停！
    roundCount++;
    int pScore = checkMatch(aiSecret, pGuess);

    // ✨ 新增：刚结束第 9 轮，即将进入第 10 轮生死局！
    if (roundCount == 9) {
        bgLayer->hide(); // 隐藏原来的静态背景

        dangerBgLayer->show();
        dangerMovie->start(); // 播放路径为 nine 的动图
        dangerBgLayer->raise(); // 让动图图层升上来

        gameContainer->raise(); // ⚠️ 极其重要：把游戏 UI 容器再次提上来，防止被动图挡住！
        if (playPlayer) playPlayer->stop();
        if (guessPlayer) guessPlayer->play();
        // 顺便改一下状态文字，增加压迫感
        statusLabel->setText("<font color='#FF0000'><b>🚨 警告：最后一次拦截机会！巨龙已锁定坐标！</b></font>");
    }

    // 更新 AI 炸弹引信

    // 更新 AI 炸弹引信
    aiBombFuse->setValue(pScore);

    QString currentLog = QString("<br><b>>>>> 第 %1 轮 <<<<</b><br>").arg(roundCount);
    currentLog += QString("你猜 [%1] -> 匹配度 %2<br>").arg(pGuess).arg(pScore);
    // ✨ 新增：第 10 轮强制失败逻辑
    if (roundCount >= 10 && pScore < 4) {
        failMovie = new QMovie(":/yesno.gif"); // 👈 确保你的资源文件名是 fail.gif
        explosionLabel->setMovie(failMovie);   // 将动图绑定到标签上
        failMovie->start();                    // 👈 启动播放！

        explosionLabel->show();
        explosionLabel->raise();
        if (guessPlayer) guessPlayer->stop();
        if (playPlayer) playPlayer->stop();
        if (failMusicPlayer) failMusicPlayer->play();
        // 确保动图盖在最上
        if (dangerMovie) {
            dangerMovie->stop();
            dangerBgLayer->hide();
        }

        // --- 删掉原本的 QMessageBox，改为使用无边框的全屏文字 ---
        QString failText = QString(
                               "【 Warning: Time Out 】\n\n"
                               "轮数已达 10 轮上限。\n"
                               "炸弹在火光中引爆了...\n\n"
                               "🔴 炸弹密码为：【 %1 】\n\n"
                               "结局：孤身生还。你活了下来，但公主已不在。请查看作战复盘分析。"
                               ).arg(aiSecret); // 👈 注意：.arg 应该在这里，且参数应该是巨龙的底牌 aiSecret


        // 每次调用前重置一下样式，确保背景透明且无边框
        storyLabel->setStyleSheet("color: #000000; "                 // 血红色字体
                                  "font-family: 'Microsoft YaHei'; "
                                  "font-size: 26px; "
                                  "font-weight: bold; "
                                  "background: transparent; "        // 👈 透明背景
                                  "border: none;");                  // 👈 不要边框
        storyLabel->setText(failText);
        storyLabel->show();
        storyLabel->raise(); // 确保文字浮在爆炸动图之上

        playerGuessInput->setEnabled(false);
        guessBtn->setEnabled(false);

        // --- 将败北复盘按钮改为黄色 ---
        reviewBtn->setText("败北复盘");
        reviewBtn->setStyleSheet(
            "QPushButton {"
            "   background-color: #000; "             // 黑色背景
            "   border: 2px solid #FFFF00; "          // 🟡 黄色边框
            "   color: #FFFF00; "                     // 🟡 黄色字体
            "   font-weight: bold; "
            "   font-size: 16px; "
            "   border-radius: 10px; "
            "}"
            "QPushButton:hover {"
            "   background-color: #FFFF00; "          // 悬停时背景变黄
            "   color: #000; "                        // 悬停时字体变黑
            "}"
            );
        reviewBtn->show();
        reviewBtn->raise();
        return; // 💥 关键：时间到了直接结束，不执行后面的 AI 动作
    }
    if (pScore == 4) {
        // ✨ 1. 替换为浪漫的 win 动图
        if (failMovie) failMovie->stop(); // 复用电影指针，停掉可能存在的其他动图
        failMovie = new QMovie(":/win.gif"); // 确保资源文件中叫 win.gif
        explosionLabel->setMovie(failMovie);
        failMovie->start();
        explosionLabel->show();
        explosionLabel->raise();
        if (guessPlayer) {
            guessPlayer->stop();
        }
if (playPlayer) playPlayer->stop();
        // 触发播放胜利音乐
        if (winPlayer) {
            winPlayer->play();
        }
        // 确保危险背景闪烁被关掉
        if (dangerMovie) dangerMovie->stop();
        if (dangerBgLayer) dangerBgLayer->hide();

        // ✨ 2. 准备史诗感的胜利文案
        QString winText = QString(
                              "【 破译成功：解除全域锁定 】\n\n"
                              "你在第 %1 轮解开了炸弹密码：【 %2 】\n"
                              "巨龙的怒吼被永远封死在崩塌的数字废墟中...\n\n"
                              "勇士，你成功救出了公主米娅。\n"
                              "结局：无数伤痕见证，勇士把公主带回国王面前。"
                              ).arg(roundCount).arg(aiSecret);

        // ✨ 3. 设置剧情文字面板的样式（半透明深色底，保证在五彩背景下白字依然清晰）
        storyLabel->setStyleSheet("color: #FFFFFF; "
                                  "font-family: 'Microsoft YaHei'; "
                                  "font-size: 26px; "
                                  "font-weight: bold; "
                                  "background-color: rgba(0, 0, 0, 120); " // 稍深一点的半透明黑底
                                  "border-radius: 15px; "
                                  "padding: 20px;");
        storyLabel->setText(""); // 先清空文字
        storyLabel->show();
        storyLabel->raise();

        // 冻结输入框
        playerGuessInput->setEnabled(false);
        playerGuessInput->setPlaceholderText("对局已结束，请查阅作战日志");
        guessBtn->setEnabled(false);

        // ✨ 4. 专属的局部“打字机”效果定时器
        int *charIndex = new int(0); // 记录当前打到第几个字了
        QTimer *winTypeTimer = new QTimer(this);
        connect(winTypeTimer, &QTimer::timeout, this, [=]() mutable {
            if (*charIndex < winText.length()) {
                // 逐字增加
                storyLabel->setText(winText.left(*charIndex + 1));
                (*charIndex)++;
            } else {
                // 字打完了，停掉定时器并清理内存
                winTypeTimer->stop();
                winTypeTimer->deleteLater();
                delete charIndex;

                // ✨ 5. 字全打完之后，才亮出胜利的复盘按钮
                reviewBtn->setText("🏆 胜利复盘");
                reviewBtn->setStyleSheet(
                    "QPushButton {"
                    "   background-color: #000; border: 2px solid #00FF41; color: #00FF41;"
                    "   font-weight: bold; font-size: 16px; border-radius: 10px;"
                    "}"
                    "QPushButton:hover { background-color: #00FF41; color: #000; }"
                    );
                reviewBtn->show();
                reviewBtn->raise();
            }
        });

        winTypeTimer->start(60); // 60毫秒打一个字，速度适中，充满娓娓道来的感觉

        return; // 💥 关键：时间到了直接结束，不执行后面的 AI 动作
    }

    // 玩家猜完，立刻轮到 AI 猜
    aiAction(currentLog);
}

void Widget::aiAction(QString currentLog) {
    // 算法升级：香农信息熵最大化模式
    QString bestGuess = aiPool[0];//先在池子里随便挑一个猜
    double maxEntropy = -1.0; // 熵的初始值设为极小？

    // 随机抽查 50 个？
    int checkCount = qMin(50, (int)aiPool.size());
    for (int i = 0; i < checkCount; i++) {
        // 随机抽一个候选数字进行推演
        QString testGuess = aiPool[QRandomGenerator::global()->bounded(aiPool.size())];

        // 统计这个猜测在 5 种反馈（0到4分）下，分别会剩下多少个数字
        int counts[5] = {0, 0, 0, 0, 0};
        for (const QString& s : aiPool) {
            int score = checkMatch(s, testGuess);
            if (score >= 0 && score <= 4) {
                counts[score]++;
            }
        }

        //  核心：计算这个数字的“香农信息熵”
        // 公式：H = -Σ (p * log2(p))
        double entropy = 0.0;
        double total = (double)aiPool.size();

        for (int score = 0; score <= 4; score++) {
            if (counts[score] > 0) {
                double probability = counts[score] / total;//计算p？
                entropy -= probability * std::log2(probability);
            }
        }

        // 寻找那个能把剩下的可能性切分得“最细、最均匀”（熵最大）的数字
        if (entropy > maxEntropy) {
            maxEntropy = entropy;
            bestGuess = testGuess;
        }
    }
    QString aiGuess = bestGuess; // 锁定这颗高智商核弹


    int aiScore = checkMatch(playerSecret, aiGuess);

    // 更新玩家炸弹引信
    playerBombFuse->setValue(aiScore);

    currentLog += QString("<font color='#ff0041'>AI 猜 [%1] -> 匹配度 %2</font><br>").arg(aiGuess).arg(aiScore);
    currentLog += "----------------------<br>";

    logLabel->setText(currentLog + logLabel->text());
    playerGuessInput->clear();

    if (aiScore == 4) {
        if (playPlayer) playPlayer->stop();
        failMovie = new QMovie(":/nine.gif");
        explosionLabel->setMovie(failMovie);
        failMovie->start(); // 启动动图播放

        explosionLabel->show();
        explosionLabel->raise(); // 确保遮罩层在最上方

        // 如果第10轮背景还在闪烁，将其停止
        if (dangerMovie) dangerMovie->stop();
        if (dangerBgLayer) dangerBgLayer->hide();
        // 使用 singleShot 延迟弹出，确保动图先显示出来
        // --- 从这里开始修改 (原本是 QTimer::singleShot) ---

        // 1. 构造警告文本
        QString warningText = QString(
                                  "【 警告：基地坐标已外泄 】\n\n"
                                  "巨龙在第 %1 轮完成了全局封锁！\n"
                                  "防御系统正在崩溃，公主的生命体征正在减弱...\n\n"
                                  "在信号彻底中断前，你是否要尝试最后拦截？"
                                  ).arg(roundCount);

        // 2. 将文本设置到我们刚才初始化的 storyLabel 上
        storyLabel->setText(warningText);
        storyLabel->show();
        storyLabel->raise(); // 👈 极其重要：确保文字浮在爆炸动图之上

        // --- 原本 578 行之后的 rescueBtn 代码保持不变 ---
        rescueBtn->setText("🚨 尝试最后拦截"); // 顺便把按钮文字改了
        rescueBtn->show();
        rescueBtn->raise();
        // 3. 这里的日志可以写得绝望一点
        logLabel->setText("<font color='red'><b>[ ERROR ] 核心即将崩塌！输入拦截指令...</b></font><br>" + logLabel->text());

        // --- 连接按钮的点击事件（只连接一次） ---
        // 用 disconnect 确保不会因为多次失败导致连接叠加
        disconnect(rescueBtn, &QPushButton::clicked, nullptr, nullptr);
        connect(rescueBtn, &QPushButton::clicked, [=](){
            // ✨ 插入点：点击按钮的一瞬间，把全屏警告文字藏起来
            storyLabel->hide();

            rescueBtn->hide(); // 原本第 597 行的代码：点击后按钮消失


            // ✨ 核心操作：从题库里随机抽一个索引
            if (riddleBank.isEmpty()) return; // 防御机制：确保题库不为空
            int riddleIdx = QRandomGenerator::global()->bounded(riddleBank.size());
            Riddle currentRiddle = riddleBank[riddleIdx];
            // 确保猜谜音乐在响

            bool ok;
            if (guessPlayer) guessPlayer->play();
            QString answer = QInputDialog::getText(this, "⚠️ 最后拦截机会",
                                                   "【 紧急补丁：脑筋急转弯 】\n\n" + currentRiddle.question,
                                                   QLineEdit::Normal, "", &ok);
// --- 玩家操作完毕，立刻关掉音乐 ---
    if (guessPlayer) {
        guessPlayer->stop();
    }
    if (failMusicPlayer) {
        failMusicPlayer->play();
    }
    // situation A: 答对了...
    if (ok && answer == currentRiddle.answer) {
        // ...

            // situation A: 答对了，劫后余生！（这里变成了动态校验题库里的答案）



                // ✨ 修改 1：将静图 ok 换成震撼的 yesno 爆炸动图
                if (failMovie) failMovie->stop(); // 停掉之前的 ni 动图
                failMovie = new QMovie(":/yesno.gif");
                explosionLabel->setMovie(failMovie);
                failMovie->start();

                // ✨ 修改 2：准备剧情文字，准备打在屏幕上
                // 找到你设置 survivalText 的地方，修改为如下格式
                QString survivalText = QString(
                                           "【 单向撤离协议已激活 】\n\n"
                                           "拦截成功！你成功覆盖了基地的自毁路径。\n\n"
                                           "但在数据洪流爆发的最后一秒，公主没能及时完成同步离线...\n\n"
                                           "🔴 炸弹密码为：【 %1 】\n\n"   // 👈 这里保留 %1
                                           "结局：孤身生还。你活了下来，但公主已不在。请查看作战复盘分析。"
                                           ).arg(aiSecret); // 👈 关键：加上这一行，把巨龙的真实底牌传进去
                QTimer::singleShot(300, this, [=](){

                    // 将文字显示在我们之前做好的全屏标签上
                    storyLabel->setText(survivalText);
                    // 💡 如果你第一页写了专门的“打字机动画函数”(比如 typeText)，这里可以换成调用那个函数

                    storyLabel->show();
                    storyLabel->raise(); // 确保文字浮在爆炸动图之上

                    playerGuessInput->setEnabled(false); // 冻结输入

                    // 亮出复盘按钮
                    reviewBtn->setText("🕯️ 任务复盘");

                    // 在你之前写的拦截成功逻辑里找到这一段并替换
                    reviewBtn->setText("🕯️ 任务复盘");

                    // 将颜色改为黄色 (#FFFF00)
                    reviewBtn->setStyleSheet(
                        "QPushButton {"
                        "   background-color: #000; "             // 黑色背景
                        "   border: 2px solid #FFFF00; "          // 🟡 黄色边框
                        "   color: #FFFF00; "                     // 🟡 黄色字体
                        "   font-weight: bold; "
                        "   font-size: 16px; "
                        "   border-radius: 10px; "
                        "}"
                        "QPushButton:hover {"
                        "   background-color: #FFFF00; "          // 悬停时背景变黄
                        "   color: #000; "                        // 悬停时字体变黑
                        "}"
                        );

                    reviewBtn->show();
                    reviewBtn->raise();
                });


            } else {
                // situation B: 没填或者答错了（拦截失败，双双殒命）

                // ✨ 1. 拦截失败，防线崩溃，必须播放大爆炸动图
                if (failMovie) {
                    failMovie->stop();
                    failMovie->setFileName(":/nine.gif"); //
                    failMovie->start();
                }

                // ✨ 2. 准备向全境宣告的“讣告”文字
                QString tragedyText = QString(
                                          "【 全境哀鸣：防线已彻底崩溃 】\n\n"
                                          "最后的拦截指令失效，巨龙的烈焰吞噬了整个基站。\n"
                                          "无人生还......\n\n"
                                          "🔴 巨龙的致命底牌实为：【 %1 】\n\n"

                                          ).arg(aiSecret);

                // ✨ 3. 延迟一瞬间，让爆炸先震撼一下视觉，再浮现死讯
                QTimer::singleShot(300, this, [=](){
                    // 设置样式：透明背景、无边框、骨灰般的灰白色字体（带点凄凉感）
                    storyLabel->setStyleSheet("color: #E0E0E0; "
                                              "font-family: 'Microsoft YaHei'; "
                                              "font-size: 26px; "
                                              "font-weight: bold; "
                                              "background: transparent; "
                                              "border: none;");
                    storyLabel->setText(tragedyText);

                    // 💡 如果你写了打字机特效函数，把上面这行换成你的打字机函数，比如 typeText(tragedyText, storyLabel);

                    storyLabel->show();
                    storyLabel->raise(); // 确保文字浮在爆炸的火光之上

                    // 冻结输入框
                    playerGuessInput->setEnabled(false);

                    // ✨ 4. 核心改动：把复盘按钮彻底隐藏！人死了没有复盘！
                    // ✨ 4. 核心改动：把复盘按钮彻底隐藏！人死了没有复盘！
                    reviewBtn->hide();
                }); // 👈 结束 QTimer::singleShot
            } // 👈 结束 else (situation B)
        }); // 👈 结束 connect(rescueBtn...)

        // 💥 缺失的关键 1：加上 return 和 结束 if 的大括号！
        return; // 既然触发了自毁逻辑，就直接 return 结束，不要再算题库了
    } // 👈 这个 } 非常重要！它关掉了前面的 if (aiScore == 4)


    // --- 👇 这里是正常的每轮循环逻辑，必须放在 if (aiScore == 4) 的外面 ---

    // 核心排除算法
    QStringList nextPool;
    for (const QString& s : aiPool) {
        if (checkMatch(s, aiGuess) == aiScore) {
            nextPool.push_back(s);
        }
    }
    aiPool = nextPool;

    // 正常更新界面的搜索空间
    statusLabel->setText(QString("当前 巨龙剩余搜索空间：<font color='#ff0041'><b>%1</b></font> 种可能").arg(aiPool.size()));
    timeLeft = 30;
    countdownLabel->setText("00:30");
    countdownLabel->setStyleSheet("color: #00FF41; font-size: 36px; font-weight: bold; background: transparent;");
    roundTimer->start(1000);

} // 👈 缺失的关键 2：这个 } 关掉了整个 Widget::aiAction 函数！


// ==========================================
// 下面是 checkMatch 函数，保持原样，但删掉最末尾多余的大括号
// ==========================================
int Widget::checkMatch(QString secret, QString guess) {
    int count = 0;
    for(int i=0; i<4; i++) {
        if(secret[i] == guess[i]) count++;
    }
    return count;
}

// 💥 缺失的关键 3：把 checkMatch 后面多余的 } 删掉！千万别留着。
// ==========================================
// 玩家超时惩罚逻辑
// ==========================================
void Widget::handleTimeout() {
    roundCount++; // 照样消耗一个回合

    // 1. 第 9 轮的死亡闪烁预警（复用你之前的逻辑）
    if (roundCount == 9) {
        if(bgLayer) bgLayer->hide();
        if(dangerBgLayer) dangerBgLayer->show();
        if(dangerMovie) dangerMovie->start();
        if(dangerBgLayer) dangerBgLayer->raise();
        if(gameContainer) gameContainer->raise();
        if (playPlayer) playPlayer->stop();
        if (guessPlayer) guessPlayer->play();
        statusLabel->setText("<font color='#FF0000'><b>🚨 警告：最后一次拦截机会！巨龙已锁定坐标！</b></font>");
    }

    // 2. 日志无情宣告：匹配度 0
    QString currentLog = QString("<br><b>>>>> 第 %1 轮 <<<<</b><br>").arg(roundCount);
    currentLog += "<font color='red'><b>[ SYSTEM ERROR：操作超时 ] -> 强制匹配度 0</b></font><br>";

    // 3. 如果第 10 轮超时，直接触发爆炸失败结局
    // 3. 如果第 10 轮超时，直接触发爆炸失败结局
    if (roundCount >= 10) {
        // ✨ 修改这里：先安全停止，再重新分配内存
        if (failMovie) {
            failMovie->stop();
        }
        failMovie = new QMovie(":/nine.gif");  // 👈 必须重新 new 一个出来
        explosionLabel->setMovie(failMovie);
        failMovie->start();

        explosionLabel->show();
        explosionLabel->raise();
        if (guessPlayer) guessPlayer->stop();
        if (playPlayer) playPlayer->stop();
        if (failMusicPlayer) failMusicPlayer->play();

        // ... 下面的代码保持原样不要动 ...

        if (dangerMovie) dangerMovie->stop();
        if (dangerBgLayer) dangerBgLayer->hide();


        // 1. 设置凄凉的灰白色字体（像骨灰一样的颜色，比纯黑在火光中更有悲剧感）
        storyLabel->setStyleSheet("color: #E0E0E0; "
                                  "font-family: 'Microsoft YaHei'; "
                                  "font-size: 26px; "
                                  "font-weight: bold; "
                                  "background: transparent; "
                                  "border: none;");

        // 2. 更改文案：超时导致双双殒命
        QString timeoutText = "【 Warning：Timeout 】\n\n"
                              "倒计时归零，最后的防线彻底崩溃。\n"
                              "炸弹在火光中引爆，烈焰吞噬了整个基站...\n\n"
                              "结局：无人生还。\n"
            ;
        storyLabel->setText(timeoutText);
        storyLabel->show();
        storyLabel->raise();

        // 3. 冻结所有输入操作
        playerGuessInput->setEnabled(false);
        guessBtn->setEnabled(false);

        // ✨ 4. 核心改动：既然人都死了，就把复盘按钮彻底隐藏！
        reviewBtn->hide();

        return; // 💥 关键：直接结束当前函数
    }

    // 4. 如果没到 10 轮，把回合交给 AI，让 AI 继续追击！
    aiAction(currentLog);
}