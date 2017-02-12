#!/usr/bin/python
# -*- coding: iso-8859-15 -*-
import threading
from subprocess import Popen, call
import sys
import signal
import os
import time

"""videothread.py: Threaded, stopable video player class"""
__author__ = "Wes Modes (wmodes@gmail.com) & SL Benz (slbenzy@gmail.com)"
__copyright__ = "2017, MIT"

# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4


OMX_CMD = ['omxplayer', '--no-osd', '--no-keys', '--refresh', '--aspect-mode fill']
CONTENT_CMD = OMX_CMD + ['--layer 4', '--dbus_name', 'org.mpris.MediaPlayer2.omxplayer1']
TRANSITION_CMD = OMX_CMD + ['--layer 5', '--dbus_name', 'org.mpris.MediaPlayer2.omxplayer2']
LOOP_CMD = OMX_CMD + ['--layer 1', '--loop', '--dbus_name', 'org.mpris.MediaPlayer2.omxplayer3']

nullin = open(os.devnull, 'r')
nullout = open(os.devnull, 'w')


class VideoThread(threading.Thread):
    """Thread class with a stop() method. The thread itself checks
    regularly for the stopped() condition."""
    _example = """
        The class takes a list containing one or more dictionaries containing
        data about the videos to be played:
            [{'name': "idle_2.mp4", # filename of the file
              'type': 'loop',       # video type: loop, transition, or content
              'start': 0.0,         # start position in video (sec)
              'length': 0.0,        # duration to play (sec)
              'layer': 1,           # omxplayer layer on which to display
              'disabled': False     # If set, record is ignored
            }]
        """

    def __init__(self, playlist=None, debug=0):
        super(VideoThread, self).__init__()
        self._stop = threading.Event()
        self._debug = debug
        self._last_debug_caller = None
        self._current_video = None
        self._player_pgid = None
        if playlist:
            self.start_sequence(playlist)

    def stop(self):
        self.__debug_("Stop flag set")
        self._stop.set()

    def stopped(self):
        return self._stop.isSet()

    def debug(self, debug_flag):
        self._debug = debug_flag

    def __debug_(self, *args, **kwargs):
        """Produce debug message, indenting if from same calling function"""
        level = 1
        if ("level" in kwargs):
            level = kwargs["level"]
        elif("l" in kwargs):
            level = kwargs["l"]
        # if (level >= self.debug):
        text = " ".join(list(map(str, args)))
        # get name of calling function
        caller = sys._getframe(1).f_code.co_name
        if (caller == self._last_debug_caller):
            print "  DEBUG: %s: %s" % (caller, text)
        else:
            print "DEBUG: %s: %s" % (caller, text)
        # save last calling function
        self._last_debug_caller = caller

    def start_sequence(self, playlist):
        if not isinstance(playlist, list):
            raise ValueError(self._example)
        for video in playlist:
            if not self.stopped():
                self.__debug_("Starting:", video)
                self.__start_video__(video)

    def __start_video__(self, video):
        if not isinstance(video, dict):
            raise ValueError(self._example)
        if 'disabled' in video and video['disabled']:
            self.__debug_("not played:", video['name'], "disabled")
            return
        self._current_video = video
        self.__debug_("name:%s type:%s start:%.1fs end:%.1fs len:%.1fs" %
                      (video['name'], video['type'], video['start'],
                       video['start']+video['length'], video['length']))
        # construct the player command
        if (video['type'] == 'loop'):
            my_cmd = " ".join(LOOP_CMD + [video['name']])
        elif (video['type'] == 'transition'):
            my_cmd = " ".join(TRANSITION_CMD + ['--pos', str(video['start']), video['name']])
        elif (video['type'] == 'content'):
            my_cmd = " ".join(CONTENT_CMD + ['--pos', str(video['start']), video['name']])
        self.__debug_("cmd:", my_cmd)
        # launch the player, saving the process handle
        try:
            proc = None
            if (_debug):
                proc = Popen(my_cmd, shell=True, preexec_fn=os.setsid, stdin=nullin)
            else:
                proc = Popen(my_cmd, shell=True, preexec_fn=os.setsid, stdin=nullin, stdout=nullout)
            # save this process group id
            self._player_pgid = os.getpgid(proc.pid)
            self.__debug_("pgid:", self._player_pgid)
            # self.__debug_("setting kill timer for %i: %i sec" % (self._player_pgid, video['length']))
            # wait in a tight loop, checking if we've received stop event or time is over
            start_time = time.time()
            while (not self.stopped() and (time.time() >= start_time + video('length'))):
                pass
            self.__stop_video__()
        except:
            self.__debug_("Unable to start video", video['name'], l=0)

    def __stop_video__(self):
        try:
            self.__debug_("Killing process %i (%s)" % (self._player_pgid, self._current_video['name']))
            os.killpg(self._player_pgid, signal.SIGTERM)
            self._player_pgid = None
        except:
            self.__debug_("Couldn't signal", self._player_pgid)
            pass
