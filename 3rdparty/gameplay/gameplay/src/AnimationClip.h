#pragma once

#include "Curve.h"
#include "Animation.h"


namespace gameplay
{
    class Animation;


    /**
     * Defines the runtime session of an Animation to be played.
     */
    class AnimationClip : public Ref
    {
        friend class AnimationController;
        friend class Animation;

    public:

        /**
         * Defines a constant for indefinitely repeating an AnimationClip.
         */
        static const unsigned int REPEAT_INDEFINITE = 0;


        /**
         * Defines an animation event listener.
         */
        class Listener
        {
            friend class AnimationClip;

        public:

            /**
             * Constructor.
             */
            Listener()
            {
            }


            /**
             * The type of animation event.
             */
            enum EventType
            {
                /**
                 * Event fired when the clip begins.
                 */
                BEGIN,

                /**
                 * Event fired when the clip ends.
                 */
                END,

                /**
                 * Event fired at a specified time during a clip update.
                 */
                TIME
            };


            /*
             * Destructor.
             */
            virtual ~Listener()
            {
            }


            /**
             * Handles when animation event occurs.
             */
            virtual void animationEvent(AnimationClip* clip, EventType type) = 0;
        };


        /**
         * Gets the AnimationClip's ID.
         *
         * @return The AnimationClip's ID.
         */
        const std::string& getId() const;

        /**
         * Gets the Animation that this AnimationClip was created from.
         *
         * @return The Animation that this clip was created from.
         */
        Animation* getAnimation() const;

        /**
         * Gets the AnimationClip's start time.
         *
         * @return The time (in milliseconds) that the AnimationClip starts playing from.
         */
        std::chrono::microseconds getStartTime() const;

        /**
         * Gets the AnimationClip's end time.
         *
         * @return The time (in milliseconds) that the AnimationClip will end.
         */
        std::chrono::microseconds getEndTime() const;

        /**
         * Gets the AnimationClip's elapsed time.
         *
         * @return The elapsed time of the AnimationClip (in milliseconds).
         */
        std::chrono::microseconds getElapsedTime() const;

        void setElapsedTime(const std::chrono::microseconds& time)
        {
            _elapsedTime = time;
        }

        /**
         * Gets the AnimationClip's duration.
         *
         * @return the AnimationClip's duration, in milliseconds.
         */
        std::chrono::microseconds getDuration() const;

        /**
         * Checks if the AnimationClip is playing.
         *
         * @return true if the AnimationClip is playing; false if the AnimationClip is not playing.
         */
        bool isPlaying() const;

        /**
         * Plays the AnimationClip.
         */
        void play(const std::chrono::microseconds& timeOffset = std::chrono::microseconds::zero());

        /**
         * Stops the AnimationClip.
         */
        void stop();

        /**
         * Pauses the AnimationClip.
         */
        void pause();

        /**
         * Adds an animation begin listener.
         *
         * @param listener The listener to be called when an AnimationClip begins.
         */
        void addBeginListener(AnimationClip::Listener* listener);

        /**
         * Removes an animation begin listener.
         *
         * @param listener The listener to be removed.
         */
        void removeBeginListener(AnimationClip::Listener* listener);

        /**
         * Adds an animation end listener.
         *
         * @param listener The listener to be called when an AnimationClip ends.
         */
        void addEndListener(AnimationClip::Listener* listener);

        /**
         * Removes an animation end listener.
         *
         * @param listener The listener to be removed.
         */
        void removeEndListener(AnimationClip::Listener* listener);

        /**
         * Adds an animation listener to be called back at the specified eventTime during the playback
         * of the AnimationClip.
         *
         * @param listener The listener to be called when the AnimationClip reaches the
         *      specified time in its playback.
         * @param eventTime The time the listener will be called during the playback of the AnimationClip.
         *      Must be between 0 and the duration of the AnimationClip.
         */
        void addListener(AnimationClip::Listener* listener, const std::chrono::microseconds& eventTime);

        /**
         * Removes an animation listener assigned to the specified eventTime.
         *
         * @param listener The listener to be removed with the specified time.
         * @param eventTime The time of the listener to be removed.
         */
        void removeListener(AnimationClip::Listener* listener, const std::chrono::microseconds& eventTime);

    private:

        static const unsigned char CLIP_IS_PLAYING_BIT = 0x01; // Bit representing whether AnimationClip is a running clip in AnimationController
        static const unsigned char CLIP_IS_STARTED_BIT = 0x02; // Bit representing whether the AnimationClip has actually been started (ie: received first call to update())
        static const unsigned char CLIP_IS_MARKED_FOR_REMOVAL_BIT = 0x20; // Bit representing whether the clip has ended and should be removed from the AnimationController.
        static const unsigned char CLIP_IS_RESTARTED_BIT = 0x40; // Bit representing if the clip should be restarted by the AnimationController.
        static const unsigned char CLIP_IS_PAUSED_BIT = 0x80; // Bit representing if the clip is currently paused.
        static const unsigned char CLIP_ALL_BITS = 0xFF; // Bit mask for all the state bits.

        /**
         * ListenerEvent.
         *
         * Internal structure used for storing the event time at which an AnimationClip::Listener should be called back.
         */
        struct ListenerEvent
        {
            /**
             * Constructor.
             */
            ListenerEvent(Listener* listener, const std::chrono::microseconds& eventTime);

            /**
             * Destructor.
             */
            ~ListenerEvent();

            /**
             * Hidden copy assignment operator.
             */
            ListenerEvent& operator=(const ListenerEvent&) = delete;

            Listener* _listener; // This listener to call back when this event is triggered.
            std::chrono::microseconds _eventTime; // The time at which the listener will be called back at during the playback of the AnimationClip.
        };


        /**
         * Constructor.
         */
        AnimationClip(const std::string& id, Animation* animation, const std::chrono::microseconds& startTime, const std::chrono::microseconds& endTime);

        /**
         * Constructor.
         */
        AnimationClip() = delete;

        /**
         * Constructor.
         */
        AnimationClip(const AnimationClip& copy) = delete;

        /**
         * Destructor.
         */
        ~AnimationClip();

        /**
         * Hidden copy assignment operator.
         */
        AnimationClip& operator=(const AnimationClip&) = delete;

        /**
         * Updates the animation with the elapsed time.
         */
        bool update(const std::chrono::microseconds& elapsedTime);

        /**
         * Handles when the AnimationClip begins.
         */
        void onBegin();

        /**
         * Handles when the AnimationClip ends.
         */
        void onEnd();

        /**
         * Determines whether the given bit is set in the AnimationClip's state.
         */
        bool isClipStateBitSet(unsigned char bit) const;

        /**
         * Sets the given bit in the AnimationClip's state.
         */
        void setClipStateBit(unsigned char bit);

        /**
         * Resets the given bit in the AnimationClip's state.
         */
        void resetClipStateBit(unsigned char bit);

        std::string _id; // AnimationClip ID.
        Animation* _animation; // The Animation this clip is created from.
        std::chrono::microseconds _startTime; // Start time of the clip.
        std::chrono::microseconds _endTime; // End time of the clip.
        unsigned char _stateBits; // Bit flag used to keep track of the clip's current state.
        std::chrono::microseconds _timeStarted; // The game time when this clip was actually started.
        std::chrono::microseconds _elapsedTime; // Time elapsed while the clip is running.
        std::vector<Listener*> _beginListeners; // Collection of begin listeners on the clip.
        std::vector<Listener*> _endListeners; // Collection of end listeners on the clip.
        std::list<ListenerEvent*> _listeners; // Ordered collection of listeners on the clip.
        std::list<ListenerEvent*>::iterator* _listenerItr; // Iterator that points to the next listener event to be triggered.
    };
}
