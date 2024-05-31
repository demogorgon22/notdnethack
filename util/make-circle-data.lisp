;;;; SPDX-FileCopyrightText: 2024 Ron Nazarov
;;;; SPDX-License-Identifier: NGPL OR GPL-2.0-or-later

;;;; This is the script used to generate the circle_data[] array in vision.c.
;;;; You do not need to run this unless you want to modify circle_data[].
;;;; How to use:
;;;; - Run with any Common Lisp implementation
;;;; - Copy/paste the output into vision.c
;;;; - Don't forget to also update MAX_RADIUS and circle_start[]

(defun circle-limit-offset (max-radius current-radius)
  (floor (sqrt (- (+ (expt max-radius 2) max-radius) (expt current-radius 2)))))

(defun make-circle-data (max-radius)
  (loop for max-radius from 1 to max-radius
	collect (loop for current-radius from 0 to max-radius
		      collect (circle-limit-offset max-radius current-radius))))

(defun print-circle-data (data)
  (let ((absolute-index 0)
	(max-radius (length (car (last data)))))
    (format t "char circle_data[] = {~%")
    (loop for line in data
	  do (loop for relative-index from 0
		   for num in line
		   do (if (= relative-index 0)
			  (format t "/*~3d*/~2d," absolute-index num)
			  (format t "~d," num))
		      (incf absolute-index))
	     (terpri))
    (format t "/*~3d*/ ~d /* should be MAX_RADIUS+1; used to terminate range loops -dlc */~%};~%"
	    absolute-index max-radius)))

(print-circle-data (make-circle-data 32))
