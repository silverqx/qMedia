#ifndef GUI_H
#define GUI_H

class QString;

namespace Utils {
    namespace Gui {
        /**
         * Open the given path with an appropriate application
         */
        bool openPath(const QString &absolutePath);
    }
}

#endif // GUI_H
