# Five Guiding misconceptions Explained

Revision: 30.08.2022 (draft)

Author: **Rumen G.Bogdanovski**

e-mail: *rumenastro@gmail.com*


I am writing this article because because I have heard these statements many  times and I had to explain them over an over. I will try to explain them without going into too much details and I am not going to provide mathematical proofs of the claims. I will try to make the explanations as simple as possible, yet providing some hooks for those who want to learn more about the matter.

## Lower RMSE means better image

This is not true. Before explaining why let me explain briefly what [RMSE](https://en.wikipedia.org/wiki/Root-mean-square_deviation) means and something that is never mentioned but essential in understanding the point - the [Nyquist Frequency](https://en.wikipedia.org/wiki/Nyquist_frequency).

RMSE stands for Root Mean Square Error and it is a measure of how spread a set of data points are around some predicted or required value. The smaller the RMSE the more concentrated the measured data points are around the required value. In case of guiding the data points are the measured drifts and the required value is 0, which means no drift.

The drift is measured by comparing the positions of [centroids](https://en.wikipedia.org/wiki/Centroid) (centers of masses) of the stellar images in the guiding frame. The fact that it takes some time to measure this offset (the exposure time) means that we miss the high frequency errors. For example if we use 1 second exposures and the seeing is moving the stars around 5 times a second we can not see this in our guiding images. What we will see is a bit blurred image and the resulting centroid positions will be the average of the centroids if we were taking 10 &times; 0.1 second exposures. So we can not see the errors above some frequency limit. This limit is called the Nyquist frequency which is one half of the measurement frequency. In theory if we measure the drift in 1 second intervals (1 Hz) we can correct transient errors with duration more than 2 seconds (0.5 Hz). All higher frequency errors will be virtually invisible to the guiding and the measured drifts will be less scattered. This is because exposure acts like a [Moving Average Low Pass Filter](https://www.analog.com/media/en/technical-documentation/dsp-book/dsp_book_ch15.pdf) with a cutoff frequency of *2 &times; Nyquist frequency*. Well, this is over simplified explanation but the principle is this.

Let us return to the example above: We have a seeing with a frequency of 5 Hz and peak to peak of 2" (amplitude 1"). In order to guide it out (assuming a perfect mount that is able to correct immediately) we need to guide with at least 10Hz (2 &times; the seeing frequency) in order to "see" the seeing errors and in this case our RMSE would be close to 1". With our perfect mount we can correct these errors immediately and the final image sharpness would be close to the theoretical sharpness of the telescope, which is let us say [FWHM](https://en.wikipedia.org/wiki/Full_width_at_half_maximum) = 1". However if we use 1 second guiding exposures these 5Hz scintillation will be almost invisible, so almost no corrections will be made and the mount being perfect we would result in RMSE close to 0", and the stars will have FWHM &approx; 2". I say RMSE close to 0" as some [aliasing](https://en.wikipedia.org/wiki/Aliasing) will occur because the error frequencies are above the Nyquist frequency and the the low pass filter frequency response is not a [Heviside step function](https://en.wikipedia.org/wiki/Heaviside_step_function).

So we ended up with RMSE &approx; 0" and FWHM &approx; 2" of the stars vs RMSE &approx; 1" and FWHM &approx; 1" of the stars. Actually this is nothing new and surprising as this is the principle used in the amateur adaptive optics. There is a small and lightweight prism inside, which can move with several tens of Hertz to make these fast corrections needed to guide out 5-10 Hz or even faster seeing errors.

Also RMSE does not say anything about the star shapes, and you may end up in oblong stars with an excellent RMSE. The thing is that if Right Ascension RMSE and Declination RMSE differ significantly the stars in the final image will be elongated in the direction of the axis with the larger RMSE. Actually the only conclusion one can draw from the RMSE values is the final image star roundness. If the ratio *RMSE RA / RMS Dec = 1* the stars will be perfectly round.

As stated above RMSE is not a measure of how precise the guiding is. RMSE is a measure of how smooth the guiding is for a given axis with a specified guiding cycle (guiding exposure). One can not compare RMSE values without specifying the exposure time and the axis. And one can not judge the final image quality by the *Total RMSE* value. Actually the *Total RMSE* value is next to meaningless.

In conclusion: If a single number should be provided related to RMSE it would be the ratio *RMSE RA / RMS Dec* as it determines the star roundness. And the worst would be the Total RMSE, as it does not provide any useful information.

## Better polar alignment means better guiding

Often it is quite the opposite. Most amateur mounts have a relatively large backlashes and close to perfect polar alignment will require Declination corrections in both directions. This means that the backlash will play a role when a correction direction changes and may lead to several overshoots before settling. In this case it is better to have a bit of polar error this will introduce a sight drift and will ensure declination corrections in one direction.

Some will argue that the drift will introduce large errors to be corrected. Actually no. If we have 10' polar alignment error we will have accumulated Dec drift of 10' in 6 hours of tracking and the drift rate will be about 0.028"/s which can be easily guided. And if the mount has no periodic error, unguided 30 sec exposure will have just about 0.8" drift error.

## Several arc minutes of polar alignment error will lead considerable field rotation

It depends, but generally it is not an issue. The bigger the field of view the worse the field rotation. Therefore let us assume that we have a relatively large field of view of 3&deg; and a total polar alignment error of 5'. The math says that with 600 sec subs at the celestial equator the stars at the edge will be elongated by 0.35" and at 60&deg; declination the elongation will be by 0.69". So with a good seeing of 1.5" it will be barely noticeable and the optical aberrations of the system at the edge of frame will have more noticeable effect.

With 1&deg; field of view at the celestial equator the star elongation will be a mere 0.12" and at 60&deg; declination will be only 0.23". Also halving the exposure time to 300 sec the numbers above are halved too.

Please note that telescope with diameter of 120mm has a resolution of approximately 1" and all the numbers above are well within the resolution of a 12cm telescope and above that the seeing is the limiting factor.

Do not be too obsessed with the polar alignment. In most of the cases polar alignment error of 5' is more than enough.

## Perfect balance is essential for a good guiding

This does not apply to Strain wave gear (Harmonic) mounts. They do not need balancing at all. They do have a way to attach counter weights but they are more to keep the center of gravity within the base, so that the telescope will not tip over rather than easing the motors.

What comes to the worm gear mounts a sight imbalance will actually make the guiding better. The thing is that they all have some backlash in both RA and Dec and if the balance is perfect slight gust of wind or the inertia of the guiding pulse can make the mount wobble withing the backlash play. To avoid that it is a good idea to have a certain amount of imbalance in both RA and Dec, so that the gravity will always clear the backlash in one direction. There is a technique called *\"East heavy mount balancing\"* used for better guiding which utilizes this principle.

## The larger the periodic error the worse the guiding
(TBD)
- symmetric vs asymmetric curve
- smoothness of the curve
