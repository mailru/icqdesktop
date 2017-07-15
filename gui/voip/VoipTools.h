#ifndef __VOIP_TOOLS_H__
#define __VOIP_TOOLS_H__

#ifndef VOIP_TOOLS_ENABLE_BB
    #define VOIP_TOOLS_ENABLE_BB
#endif

namespace voipTools
{
    inline void showBounds(QPainter& _painter, const QRect& _rc, const QColor& borderColor)
    {
        const QPen wasPen = _painter.pen();
        _painter.setPen  (borderColor);
        _painter.fillRect(QRect(_rc.left(), _rc.top(), _rc.width() - 1, _rc.height() - 1), QColor(0, 0, 255, 100));
        _painter.drawRect(QRect(_rc.left(), _rc.left(), _rc.width() - 1, _rc.height() - 1));
        _painter.setPen(wasPen);
    }

    template<typename __Base> class BoundBox : public __Base
    {
    public:

		// It is cool solution for this class, but i cannot compile it.
		//template<typename... T2>
		//	BoundBox(T2... params)	: __Base(params...) { }

        BoundBox(const QColor& borderColor = QColor(0, 255, 0, 255)) :__Base(), borderColor_(borderColor) { }

		template<typename Param1> BoundBox(Param1 par1, const QColor& borderColor = QColor(0, 255, 0, 255))
			:__Base(par1), borderColor_(borderColor) { }

        template<typename Param1, typename Param2> BoundBox(
            Param1 par1,
            Param2 par2,
            const QColor& borderColor = QColor(0, 255, 0, 255))
            :__Base(par1, par2), borderColor_(borderColor) { }

		template<typename Param1, typename Param2, typename Param3> BoundBox(
			Param1 par1,
			Param2 par2,
			Param3 par3,
            const QColor& borderColor = QColor(0, 255, 0, 255))
			: __Base(par1, par2, par3), borderColor_(borderColor) { }

		template<typename Param1, typename Param2, typename Param3, typename Param4> BoundBox(
			Param1 par1,
			Param2 par2,
			Param3 par3,
			Param4 par4,
            const QColor& borderColor = QColor(0, 255, 0, 255))
			: __Base(par1, par2, par3, par4), borderColor_(borderColor) { }
		
        template<typename Param1, typename Param2, typename Param3, typename Param4, typename Param5> BoundBox(
            Param1 par1,
            Param2 par2,
            Param3 par3,
            Param4 par4,
            Param5 par5,
            const QColor& borderColor = QColor(0, 255, 0, 255))
            :__Base(par1, par2, par3, par4, par5), borderColor_(borderColor) { }

    protected:
        void paintEvent(QPaintEvent* _e) override
        {
            __Base::paintEvent(_e);
#if defined(_DEBUG) && defined(VOIP_TOOLS_ENABLE_BB)
            QPainter painter(this);
            voipTools::showBounds((painter), __Base::rect(), borderColor_);
#endif
        }
        QColor borderColor_;
    };

}

#endif//__VOIP_TOOLS_H__