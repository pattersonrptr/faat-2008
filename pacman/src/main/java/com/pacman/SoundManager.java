package com.pacman;

import javax.sound.sampled.*;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * Gerenciador de som que sintetiza todos os áudios programaticamente,
 * sem arquivos externos. Usa javax.sound.sampled.
 *
 * Canais independentes:
 *  - Música de fundo (loop da sirene dos fantasmas)
 *  - Efeitos sonoros (waka, power, morte, comer fantasma, fruta)
 */
public class SoundManager {

    private static final int SAMPLE_RATE = 44100;

    private float musicVolume  = 0.6f;  // 0.0 – 1.0
    private float effectVolume = 0.8f;
    private boolean musicMuted  = false;
    private boolean effectMuted = false;

    private Thread    musicThread;
    private volatile boolean musicRunning = false;
    private volatile boolean musicPaused  = false;

    private final ExecutorService sfxPool = Executors.newCachedThreadPool(r -> {
        Thread t = new Thread(r);
        t.setDaemon(true);
        return t;
    });

    // ---- Controles de volume ----

    public float getMusicVolume()   { return musicVolume; }
    public float getEffectVolume()  { return effectVolume; }
    public boolean isMusicMuted()   { return musicMuted; }
    public boolean isEffectMuted()  { return effectMuted; }

    public void setMusicVolume(float v)  { musicVolume  = clamp(v); }
    public void setEffectVolume(float v) { effectVolume = clamp(v); }
    public void setMusicMuted(boolean m) { musicMuted  = m; }
    public void setEffectMuted(boolean m){ effectMuted = m; }

    private float clamp(float v) { return Math.max(0f, Math.min(1f, v)); }

    // ---- Música de fundo ----

    public void startMusic() {
        stopMusic();
        musicRunning = true;
        musicPaused  = false;
        musicThread  = new Thread(() -> {
            // Sirene: alterna entre duas frequências (fiel ao original)
            double[] freqs = { 200, 250, 220, 270 };
            int idx = 0;
            while (musicRunning) {
                if (!musicPaused && !musicMuted) {
                    playTone(freqs[idx % freqs.length], 120, musicVolume, true);
                    idx++;
                }
                try { Thread.sleep(130); } catch (InterruptedException e) { break; }
            }
        });
        musicThread.setDaemon(true);
        musicThread.start();
    }

    public void pauseMusic()  { musicPaused = true; }
    public void resumeMusic() { musicPaused = false; }

    public void stopMusic() {
        musicRunning = false;
        if (musicThread != null) {
            musicThread.interrupt();
            musicThread = null;
        }
    }

    // ---- Efeitos sonoros ----

    /** Waka-waka: dois bipes curtos alternados */
    public void playWaka(boolean open) {
        if (effectMuted) return;
        sfxPool.submit(() -> playTone(open ? 440 : 370, 50, effectVolume, false));
    }

    /** Power pellet: som descendente */
    public void playPowerPellet() {
        if (effectMuted) return;
        sfxPool.submit(() -> {
            for (int f = 600; f >= 200; f -= 40) {
                playTone(f, 25, effectVolume, false);
            }
        });
    }

    /** Fantasma comido: som ascendente */
    public void playEatGhost() {
        if (effectMuted) return;
        sfxPool.submit(() -> {
            int[] notes = { 300, 400, 500, 600, 800 };
            for (int n : notes) playTone(n, 40, effectVolume, false);
        });
    }

    /** Morte do Pac-Man: descida cromática característica */
    public void playDeath() {
        if (effectMuted) return;
        sfxPool.submit(() -> {
            int[] freqs = { 494, 466, 440, 415, 392, 370, 349, 330, 311, 294, 277, 262 };
            for (int f : freqs) playTone(f, 80, effectVolume, false);
        });
    }

    /** Coleta de fruta */
    public void playFruit() {
        if (effectMuted) return;
        sfxPool.submit(() -> {
            int[] notes = { 523, 659, 784, 1047 };
            for (int n : notes) playTone(n, 60, effectVolume, false);
        });
    }

    /** Início de fase */
    public void playLevelStart() {
        if (effectMuted) return;
        sfxPool.submit(() -> {
            int[] melody = { 262, 330, 392, 523, 392, 523, 659 };
            for (int n : melody) {
                playTone(n, 100, effectVolume, false);
                try { Thread.sleep(10); } catch (InterruptedException ignored) {}
            }
        });
    }

    // ---- Motor de síntese ----

    /**
     * Reproduz uma onda quadrada simples na frequência e duração fornecidas.
     * @param freq      frequência em Hz
     * @param durationMs duração em milissegundos
     * @param volume    0.0 – 1.0
     * @param square    true = onda quadrada, false = senoide suavizada
     */
    private void playTone(double freq, int durationMs, float volume, boolean square) {
        try {
            int numSamples = SAMPLE_RATE * durationMs / 1000;
            byte[] buf = new byte[numSamples];

            for (int i = 0; i < numSamples; i++) {
                double angle = 2.0 * Math.PI * i * freq / SAMPLE_RATE;
                double sample = square
                        ? (Math.sin(angle) >= 0 ? 1.0 : -1.0)
                        : Math.sin(angle);
                // Envelope rápido para evitar clique
                double env = Math.min(1.0, Math.min(i / 50.0, (numSamples - i) / 50.0));
                buf[i] = (byte) (sample * env * 127 * volume);
            }

            AudioFormat fmt = new AudioFormat(SAMPLE_RATE, 8, 1, true, false);
            try (SourceDataLine line = AudioSystem.getSourceDataLine(fmt)) {
                line.open(fmt, numSamples);
                line.start();
                line.write(buf, 0, buf.length);
                line.drain();
            }
        } catch (LineUnavailableException ignored) {}
    }

    public void shutdown() {
        stopMusic();
        sfxPool.shutdownNow();
    }
}
