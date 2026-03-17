#include "Music.h"

#include "REALITY.FC.h"
#include "audioPlayer.h"
#include "st3play.h"

namespace Music
{
    namespace
    {
        struct sync
        {
            unsigned short order_and_row;
            unsigned short syncnumber;
        };

        const sync syncdata[] = {
            { 0x0000, 0 }, { 0x0200, 1 }, { 0x0300, 2 },  { 0x032f, 3 },

            { 0x042f, 4 }, { 0x052f, 5 }, { 0x062f, 6 },  { 0x072f, 7 },

            { 0x082f, 8 }, { 0x0900, 9 }, { 0x0d00, 10 }, { 0x3d00, 1 },

            { 0x3f00, 2 }, { 0x4100, 3 }, { 0x4200, 4 },
        };

        int dis_frame_start = 0;
    }

    int sync()
    {
        unsigned short order = 0;
        unsigned short row = 0;
        unsigned int frame = 0;

        st3play::GetOrderRowAndFrame(&order, &row, &frame);

        unsigned short order_and_row = (order << 8) | row;

        for (int i = 1; i <= static_cast<int>(sizeof(syncdata) / sizeof(syncdata[0])); i++)
        {
            if (syncdata[i].order_and_row >= order_and_row)
            {
                return syncdata[i - 1].syncnumber;
            }
        }

        AudioPlayer::Update();

        return 0;
    }

    void start(Song song_idx, int start_order)
    {
        // reality_fc is unsigned char[]; casting to unsigned int* is unaligned UB on MIPS.
        // The table at the front is little-endian uint32 offsets — read byte-by-byte.
        const size_t idx = static_cast<size_t>(song_idx);
        const unsigned char * p = Data::reality_fc + idx * 4;
        const unsigned int offset = (unsigned int)p[0] | ((unsigned int)p[1] << 8)
                                  | ((unsigned int)p[2] << 16) | ((unsigned int)p[3] << 24);

        st3play::PlaySong(Data::reality_fc + offset, Data::reality_fc_size, 1, 44100, start_order);
    }

    void end()
    {
        st3play::Close();
    }

    int getPlusFlags()
    {
        AudioPlayer::Update();

        return st3play::GetPlusFlags();
    }

    int getRow()
    {
        AudioPlayer::Update();

        return st3play::GetRow();
    }

    int getOrder()
    {
        AudioPlayer::Update();

        return st3play::GetOrder();
    }

    void setFrame(int frame)
    {
        dis_frame_start = st3play::GetFrame() - frame;
    }

    int getFrame()
    {
        AudioPlayer::Update();

        return st3play::GetFrame() - dis_frame_start;
    }

}