#pragma once

namespace Ui
{
    class MessagesScrollArea;

    namespace Selection
    {
        class SelectionPanel
            : public QFrame
        {
            Q_OBJECT
        public:
            SelectionPanel(MessagesScrollArea* _messages, QWidget* _parent);

        private:
            void closePanel();

        private:
            MessagesScrollArea* messages_;
        };
    }
}
