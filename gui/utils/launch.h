namespace launch
{
    class CommandLineParser
    {
        QString executable_;

        bool isUlrCommand_;

        QString urlCommand_;

    public:

        CommandLineParser(int _argc, char* _argv[]);

        ~CommandLineParser();

        bool isUrlCommand() const;

        const QString& getUrlCommand() const;

        const QString& getExecutable() const;
    };

    int main(int argc, char *argv[]);
}
