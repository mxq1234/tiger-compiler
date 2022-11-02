# Lab2 #
### 1. 怎样处理注释： ###
 &ensp;&ensp;&ensp;&ensp;使用开始条件COMMENT，当匹配到 <INITIAL\>/* 的时候，切换到StartCondition__::COMMENT下的迷你扫描器，即：

     /* comment */
    "/*" { adjust(); comment_level_ = 1; begin(StartCondition__::COMMENT); }

 &ensp;&ensp;&ensp;&ensp;此时，使用为COMMENT定义的规则进行匹配。由于存在注释多级嵌套的问题，因此使用scanner.h中定义的成员变量comment\_level\_来记录注释嵌套到第几层。COMMENT刚刚begin时，此时已经匹配到一个 /* ，comment\_level\_ 初始化为1。之后每当遇到 /* ，就让comment\_level\_加1。遇到*\，则减1，减为0的时候说明已经跳出了所有的注释嵌套，此时切换回INITIAL。

    <COMMENT> {
    "*/" {
        adjust();
        comment_level_--;
        if(!comment_level_)  begin(StartCondition__::INITIAL); 
    }
    "/*" { adjust(); comment_level_++; }
    \n { adjust(); errormsg_->Newline(); }
    <<EOF>> { adjust(); errormsg_->Error(errormsg_->tok_pos_, "EOF in COMMENT"); }
    . { adjust(); }
	}

### 2. 怎样处理字符串： ###
&ensp;&ensp;&ensp;&ensp;使用开始条件STR，当匹配到<INITIAL\>\\"的时候，切换到StartCondition__::STR下的迷你扫描器，即：

     /* string */
    \" { adjust(); string_buf_ = ""; begin(StartCondition__::STR); }
 &ensp;&ensp;&ensp;&ensp;此时，使用为STR定义的规则进行匹配。由于存在转义序列的问题，因此需要逐个匹配字符，针对转义序列作特殊处理。使用scanner.h中定义的成员变量string\_buf\_来缓存每一个匹配和处理过的字符，最终将其作为setMatched()的参数，即可将返回Parser::STRING时匹配得到的字符串设置为处理好的字符串。使用scanner.h中定义的成员函数adjustStr()来调整char_pos_，即可使返回Parser::STRING时字符串的开始位置仍然为第一个引号开始的位置。

    <STR> {
    \\[ \n\t\r] { adjustStr(); begin(StartCondition__::IGNORE); }
    \\n { adjustStr(); string_buf_ += '\n'; }
    \\t { adjustStr(); string_buf_ += '\t'; }
    \\^[A-Z] { adjustStr(); string_buf_ += (char)(matched()[2] - 'A' + 1); }
    \\^\[ { adjustStr(); string_buf_ += (char)0x1B; }
    \\^\\ { adjustStr(); string_buf_ += (char)0x1C; }
    \\^\] { adjustStr(); string_buf_ += (char)0x1D; }
    \\^^ { adjustStr(); string_buf_ += (char)0x1E; }
    \\^_ { adjustStr(); string_buf_ += (char)0x1F; }
    \\[0-9]{3} { adjustStr(); string_buf_ += (char)atoi(matched().substr(1).c_str()); }
    \\\" { adjustStr(); string_buf_ += '\"'; }
    \\\\ { adjustStr(); string_buf_ += '\\'; }
    \" {
        adjustStr();
        setMatched(string_buf_);
        begin(StartCondition__::INITIAL);
        return Parser::STRING;
    }
    ^\\ { adjust(); errormsg_->Error(errormsg_->tok_pos_, "illegal char sequence in STR"); }
    \n { adjustStr(); errormsg_->Newline(); }
    <<EOF>> { adjust(); errormsg_->Error(errormsg_->tok_pos_, "EOF in STR"); }
    . { adjustStr(); string_buf_ += matched(); }
	}
&ensp;&ensp;&ensp;&ensp;Tiger允许的转义序列有\n, \t, \^c, \ddd, \", \\, \f_\_\_f\。对于前六种，匹配到后，会向string\_buf\_中添加相应的真正的字符。如果匹配到 \\[ \n\t\r]，即\f\_\_\_f\，切换到StartCondition__::IGNORE下的迷你扫描器，即：

     /* ignore */
    <IGNORE> {
    [\n\t\r ] { adjustStr(); }
    \\ { adjustStr(); begin(StartCondition__::STR); }
    \n { adjustStr(); errormsg_->Newline(); }
    <<EOF>> { adjust(); errormsg_->Error(errormsg_->tok_pos_, "EOF in \\f___f\\"); }
    . { adjust(); errormsg_->Error(errormsg_->tok_pos_, "illegal char in \\f___f\\"); }
    }

&ensp;&ensp;&ensp;&ensp;当匹配到 \\ 时，会切回STR，否则忽略所有的[ \n\t\r]字符序列。在STR的迷你扫描器中，当扫描到可打印字符、空白符时，直接加入string\_buf\_。当匹配到 \" 时，会切回INITIAL，并设置matched为string\_buf\_中的值，返回Parser::STRING，至此字符串匹配完成。


### 3. 怎样处理错误： ###
&ensp;&ensp;&ensp;&ensp;第一个错误是INITIAL下，出现非法输入，由于是从上到下匹配，所以在经历所有合法匹配之后，用.来匹配除了\n外的所有字符，如果在此处匹配成功了，那么这是一个非法token，即：

    <INITIAL>. { adjust(); errormsg_->Error(errormsg_->tok_pos_, "illegal token"); }

&ensp;&ensp;&ensp;&ensp;对于\n，则直接添加新行：

    <INITIAL>\n { adjust(); errormsg_->Newline(); }

&ensp;&ensp;&ensp;&ensp;第二个错误是STR下，出现非法转义序列，同上，在经历所有合法的转义序列匹配后，用^\\来匹配以转义字符开头的所有字符序列，如果在此处匹配成功了，那么这个\\的使用是非法的，即：

    <STR>^\\ { adjust(); errormsg_->Error(errormsg_->tok_pos_, "illegal char sequence in STR"); }

&ensp;&ensp;&ensp;&ensp;第三个错误是IGNORE下，出现非[\n \t\r]字符，同上，在匹配[ \n\t\r]和 \\ 后，用.来匹配除了\n外的所有字符，如果在此处匹配成功了，那么这是一个非法字符，即：

    <IGNORE>. { adjust(); errormsg_->Error(errormsg_->tok_pos_, "illegal char in \\f___f\\"); }

&ensp;&ensp;&ensp;&ensp;第四种错误是非INITIAL下，EOF的错误。

### 4. 怎样处理EOF： ###
&ensp;&ensp;&ensp;&ensp;在非INITIAL下，正常的文件输入不应该在这里终止，因此如果用<<EOF\>\>匹配到了EOF，则发生了错误：

    <COMMENT><<EOF>> { adjust(); errormsg_->Error(errormsg_->tok_pos_, "EOF in COMMENT"); }
	<STR><<EOF>> { adjust(); errormsg_->Error(errormsg_->tok_pos_, "EOF in STR"); }
	<IGNORE><<EOF>> { adjust(); errormsg_->Error(errormsg_->tok_pos_, "EOF in \\f___f\\"); }

&ensp;&ensp;&ensp;&ensp;在INITIAL下，EOF是正常的。

### 5. 词法分析器其他有趣的特点： ###
&ensp;&ensp;&ensp;&ensp;词法分析器会将\^c转换为真实的控制字符，并且c不局限于大写字母，实现的时候\^c匹配包括了所有的控制字符，例如ESC的\^[。词法分析器可以在scanner.h中定义成员变量和成员函数，作为lex action中的“全局变量”和“全局函数”。词法分析器不会去识别负的整形字面量，而是将其解析为两个单词......