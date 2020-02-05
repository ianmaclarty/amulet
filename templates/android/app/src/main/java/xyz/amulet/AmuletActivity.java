// Copyright 2019 Ian MacLarty
package xyz.amulet;

import android.app.PendingIntent;
import android.app.Activity;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Looper;
import android.util.Log;
import android.view.WindowManager;
import android.content.*;
import android.content.res.*;
import android.media.AudioTrack;
import android.media.AudioFormat;
import android.media.AudioAttributes;

import android.opengl.GLSurfaceView;
import android.view.*;

import android.graphics.Color;
import android.widget.*;

import android.util.DisplayMetrics;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.opengles.GL10;

import java.util.*;
import org.json.*;

//import com.google.android.gms.ads.*;
//BILLING import com.android.billingclient.api.*;
import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.api.GoogleApiClient;
import com.google.android.gms.games.Games;

public class AmuletActivity extends Activity
        implements
        GoogleApiClient.ConnectionCallbacks,
        GoogleApiClient.OnConnectionFailedListener
        //BILLING ,PurchasesUpdatedListener
        {
    AmuletView view = null;
    //AdView adView = null;
    boolean requestAdVisible = false;
    boolean adInLayout = false;
    boolean adIsLoaded = false;
    Object adLock = new Object();
    RelativeLayout.LayoutParams adLayoutParams = null;
    int adHeight;
    GoogleApiClient googleApiClient = null;
    int googleApiClientConnectionAttempts = 0;

    static AmuletActivity singleton;

    static Object pendingProductIdLock = new Object();
    static String pendingProductId;
    RelativeLayout layout;

    AudioTrack audioTrack = null;
    int audioBufferSize = 2048;
    Thread audioThread = null;
    volatile boolean is_running = true;
    volatile boolean is_paused = false;

    //BILLING BillingClient billingClient = null;
    //BILLING volatile boolean billingConnected = false;
    //BILLING List<SkuDetails> billingSkuDetails = null;
    //BILLING String lastPurchaseProductId = null;

    public AmuletActivity() {
        singleton = this;
    }

    private void setupAudio() {
        final int sampleRate = 44100;
        audioBufferSize = AudioTrack.getMinBufferSize(sampleRate, AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_FLOAT);
        final int numFrames = audioBufferSize / 4;
        final int halfNumFrames = numFrames / 2;
        audioTrack = new AudioTrack.Builder()
             .setAudioAttributes(new AudioAttributes.Builder()
                      .setUsage(AudioAttributes.USAGE_GAME)
                      .setContentType(AudioAttributes.CONTENT_TYPE_UNKNOWN)
                      .build())
             .setAudioFormat(new AudioFormat.Builder()
                     .setEncoding(AudioFormat.ENCODING_PCM_FLOAT)
                     .setSampleRate(sampleRate)
                     .setChannelMask(AudioFormat.CHANNEL_OUT_STEREO)
                     .build())
             .setTransferMode(AudioTrack.MODE_STREAM)
             .setBufferSizeInBytes(audioBufferSize)
             .build();
        audioTrack.play();
        audioThread = new Thread(new Runnable() {
            float[] buffer = new float[halfNumFrames];
            public void run() {
                while (is_running) {
                    if (!is_paused) {
                        jniFillAudioBuffer(buffer, halfNumFrames);
                        audioTrack.write(buffer, 0, halfNumFrames, AudioTrack.WRITE_BLOCKING);
                    } else {
                        try {
                            Thread.sleep(10);
                        } catch (InterruptedException e) {};
                    }
                }
            }
        });
        audioThread.start();
    }
 
    @Override
    protected void onCreate(Bundle bundle) {
        super.onCreate(bundle);

        layout = new RelativeLayout(this);
        layout.setLayoutParams(new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));

        // XXX needed?
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);

        //Create and set GL view (OpenGL View)
        view = new AmuletView(getApplication());

        layout.addView(view);

        setContentView(layout);

        setupAudio();

        //BILLING billingClient = BillingClient.newBuilder(this).enablePendingPurchases().setListener(this).build();
        //BILLING final BillingClientStateListener billingClientListener[] = new BillingClientStateListener[1];
        //BILLING billingClientListener[0] = new BillingClientStateListener() {
        //BILLING     @Override
        //BILLING     public void onBillingSetupFinished(BillingResult billingResult) {
        //BILLING         if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK) {
        //BILLING             billingConnected = true;
        //BILLING             Purchase.PurchasesResult pRes = billingClient.queryPurchases(BillingClient.SkuType.INAPP);
        //BILLING             List<Purchase> purchases = pRes.getPurchasesList();
        //BILLING             if (purchases != null) {
        //BILLING                 for (Purchase p : purchases) {
        //BILLING                     if (!p.isAcknowledged() && p.getPurchaseState() == Purchase.PurchaseState.PURCHASED) {
        //BILLING                         handleTransactionUpdated(p.getSku(), "purchased", p.getPurchaseToken());
        //BILLING                     }
        //BILLING                 }
        //BILLING             }
        //BILLING         } else {
        //BILLING             billingConnected = false;
        //BILLING         }
        //BILLING     }
        //BILLING     @Override
        //BILLING     public void onBillingServiceDisconnected() {
        //BILLING         billingConnected = false;
        //BILLING         try {
        //BILLING             if (billingClient == null) {
        //BILLING                 billingClient = BillingClient.newBuilder(AmuletActivity.this).enablePendingPurchases().setListener(AmuletActivity.this).build();
        //BILLING             }
        //BILLING             if (billingClient != null) {
        //BILLING                 billingClient.startConnection(billingClientListener[0]);
        //BILLING             }
        //BILLING         } catch (Exception e) {
        //BILLING             // ignore
        //BILLING             Log.i("AMULET", "error handling billing service disconnect: " + e.getMessage());
        //BILLING         }
        //BILLING     }
        //BILLING };
        //BILLING billingClient.startConnection(billingClientListener[0]);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        //BILLING if (billingClient != null) { 
        //BILLING     billingClient.endConnection();
        //BILLING     billingClient = null;
        //BILLING     billingConnected = false;
        //BILLING }
        jniTeardown();
        is_running = false;
    }

    @Override protected void onPause() {
        super.onPause();
        view.onPause();
        jniPause();
        is_paused = true;
    }

    @Override protected void onResume() {
        super.onResume();
        view.onResume();
        jniResume();
        is_paused = false;
    }

    private static class AmuletView extends GLSurfaceView {
        private static String TAG = "AMULET";

        public AmuletView(Context context) {
            super(context);
            setPreserveEGLContextOnPause(true);
            setEGLContextFactory(new ContextFactory());
            setEGLConfigChooser(new ConfigChooser());
            setRenderer(new AMRenderer());
            Context appContext = context;
            Configuration config = appContext.getResources().getConfiguration();
            String lang = config.locale.getLanguage().toLowerCase();
            String country = config.locale.getCountry();
            if (country != null && country.length() > 0) lang += "-" + country.toUpperCase();
            jniInit(context.getAssets(), appContext.getFilesDir().getPath() + "/", lang);
        }

        private static class ContextFactory implements GLSurfaceView.EGLContextFactory {
            private static int EGL_CONTEXT_CLIENT_VERSION = 0x3098;
            public EGLContext createContext(EGL10 egl, EGLDisplay display, EGLConfig eglConfig) {
                checkEglError("before eglCreateContext", egl);
                int[] attrib_list = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL10.EGL_NONE };
                EGLContext context = egl.eglCreateContext(display, eglConfig, EGL10.EGL_NO_CONTEXT, attrib_list);
                checkEglError("after eglCreateContext", egl);
                return context;
            }

            public void destroyContext(EGL10 egl, EGLDisplay display, EGLContext context) {
                egl.eglDestroyContext(display, context);
            }
        }

        private static void checkEglError(String prompt, EGL10 egl) {
            int error;
            while ((error = egl.eglGetError()) != EGL10.EGL_SUCCESS) {
                Log.e(TAG, String.format("%s: EGL error: 0x%x", prompt, error));
            }
        }

        @Override
        public void onWindowFocusChanged(boolean hasFocus) {
            super.onWindowFocusChanged(hasFocus);
            if (hasFocus) {
                setSystemUiVisibility(
                        SYSTEM_UI_FLAG_LAYOUT_STABLE
                        | SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                        | SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                        | SYSTEM_UI_FLAG_HIDE_NAVIGATION
                        | SYSTEM_UI_FLAG_FULLSCREEN
                        | SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
            }
        }

        @Override
        public boolean onTouchEvent(MotionEvent e) {
            int action = e.getActionMasked();
            switch (action) {
                case MotionEvent.ACTION_DOWN:
                case MotionEvent.ACTION_POINTER_DOWN: {
                    int i = e.getActionIndex();
                    final int id = e.getPointerId(i);
                    final float x = e.getX(i);   
                    final float y = e.getY(i);   
                    queueEvent(new Runnable() {
                        public void run() {
                            jniTouchDown(id, x, y);
                        }
                    });
                    break;
                }
                case MotionEvent.ACTION_UP:
                case MotionEvent.ACTION_POINTER_UP:
                case MotionEvent.ACTION_CANCEL:
                {
                    int i = e.getActionIndex();
                    final int id = e.getPointerId(i);
                    final float x = e.getX(i);   
                    final float y = e.getY(i);   
                    queueEvent(new Runnable() {
                        public void run() {
                            jniTouchUp(id, x, y);
                        }
                    });
                    break;
                }
                case MotionEvent.ACTION_MOVE:
                {
                    // For move events getActionIndex doesn't seem to return the index that moved,
                    // instead it always returns 0. So we need to iterate through all pointers.
                    for (int i = 0; i < e.getPointerCount(); i++) {
                        final int id = e.getPointerId(i);
                        final float x = e.getX(i);   
                        final float y = e.getY(i);   
                        queueEvent(new Runnable() {
                            public void run() {
                                jniTouchMove(id, x, y);
                            }
                        });
                    }
                    break;
                }
            }
            return true;
        }
    }

    private static class ConfigChooser implements GLSurfaceView.EGLConfigChooser {

        private EGLConfig tryConfig(EGL10 egl, EGLDisplay display, int[] config) {
            int[] num_config = new int[1];
            egl.eglChooseConfig(display, config, null, 0, num_config);

            int numConfigs = num_config[0];

            if (numConfigs <= 0) return null;

            EGLConfig[] configs = new EGLConfig[numConfigs];
            egl.eglChooseConfig(display, config, configs, numConfigs, num_config);

            return configs[0];
        }

        public EGLConfig chooseConfig(EGL10 egl, EGLDisplay display) {
            int EGL_OPENGL_ES2_BIT = 4;
            EGLConfig config;

            config = tryConfig(egl, display, 
                new int[] {
                    EGL10.EGL_RED_SIZE, 8,
                    EGL10.EGL_GREEN_SIZE, 8,
                    EGL10.EGL_BLUE_SIZE, 8,
                    EGL10.EGL_DEPTH_SIZE, 24,
                    EGL10.EGL_STENCIL_SIZE, 8,
                    EGL10.EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                    EGL10.EGL_NONE
                });
            if (config != null) return config;

            config = tryConfig(egl, display, 
                new int[] {
                    EGL10.EGL_RED_SIZE, 8,
                    EGL10.EGL_GREEN_SIZE, 8,
                    EGL10.EGL_BLUE_SIZE, 8,
                    EGL10.EGL_DEPTH_SIZE, 16,
                    EGL10.EGL_STENCIL_SIZE, 8,
                    EGL10.EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                    EGL10.EGL_NONE
                });
            if (config != null) return config;

            config = tryConfig(egl, display, 
                new int[] {
                    EGL10.EGL_RED_SIZE, 4,
                    EGL10.EGL_GREEN_SIZE, 4,
                    EGL10.EGL_BLUE_SIZE, 4,
                    EGL10.EGL_DEPTH_SIZE, 16,
                    EGL10.EGL_STENCIL_SIZE, 8,
                    EGL10.EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                    EGL10.EGL_NONE
                });
            if (config != null) return config;

            config = tryConfig(egl, display, 
                new int[] {
                    EGL10.EGL_RED_SIZE, 8,
                    EGL10.EGL_GREEN_SIZE, 8,
                    EGL10.EGL_BLUE_SIZE, 8,
                    EGL10.EGL_DEPTH_SIZE, 16,
                    EGL10.EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                    EGL10.EGL_NONE
                });
            if (config != null) return config;

            config = tryConfig(egl, display, 
                new int[] {
                    EGL10.EGL_RED_SIZE, 4,
                    EGL10.EGL_GREEN_SIZE, 4,
                    EGL10.EGL_BLUE_SIZE, 4,
                    EGL10.EGL_DEPTH_SIZE, 16,
                    EGL10.EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                    EGL10.EGL_NONE
                });
            if (config != null) return config;

            config = tryConfig(egl, display, 
                new int[] {
                    EGL10.EGL_RED_SIZE, 4,
                    EGL10.EGL_GREEN_SIZE, 4,
                    EGL10.EGL_BLUE_SIZE, 4,
                    EGL10.EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                    EGL10.EGL_NONE
                });
            if (config != null) return config;

            throw new RuntimeException("No useful EGL configs found!");
        }
    }

    private static class AMRenderer implements GLSurfaceView.Renderer {
        public void onDrawFrame(GL10 gl) {
            jniStep();
        }

        public void onSurfaceChanged(GL10 gl, int width, int height) {
            Log.i("AMULET", "surface changed " + width + " " + height);
            jniResize(width, height);
        }

        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            // this will also be called if glcontext was lost
            Log.i("AMULET", "surface created");
            jniSurfaceCreated();
        }
    }

    // --- Admob -----------------------

    /*
    public static void cppInitAds(String unitId0) {
        final String unitId = unitId0;
        Log.i("AMULET", "called cppInitAds");
        singleton.runOnUiThread(new Runnable() {
            public void run() {
                synchronized (singleton.adLock) {
                    if (singleton.adView != null) return;
                    singleton.adView = new AdView(singleton);
                    singleton.adView.setAdListener(new AdListener() {
                        @Override
                        public void onAdFailedToLoad(int i) {
                            super.onAdFailedToLoad(i);
                            singleton.adIsLoaded = false;
                        }

                        @Override
                        public void onAdLoaded() {
                            super.onAdLoaded();
                            singleton.adIsLoaded = true;
                            if (singleton.requestAdVisible) {
                                cppSetAdVisible(1);
                                singleton.requestAdVisible = false;
                            }
                        }
                    });
                    singleton.adView.setAdSize(AdSize.SMART_BANNER);
                    singleton.adView.setAdUnitId(unitId);
                    singleton.adView.setBackgroundColor(Color.TRANSPARENT);
                    singleton.adLayoutParams =
                            new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.WRAP_CONTENT,
                                    RelativeLayout.LayoutParams.WRAP_CONTENT);
                    singleton.adLayoutParams.addRule(RelativeLayout.ALIGN_PARENT_TOP);
                    singleton.adLayoutParams.addRule(RelativeLayout.CENTER_HORIZONTAL);
                }
            }
        });
    }

    public static void cppSetAdVisible(int visible0) {
        Log.i("AMULET", "called cppSetAdVisible");
        final int visible = visible0;
        singleton.runOnUiThread(new Runnable() {
            public void run() {
                synchronized (singleton.adLock) {
                    singleton.requestAdVisible = visible == 1 ? true : false;
                    if (singleton.adView == null) return;
                    if (visible == 1) {
                        if (singleton.adInLayout) return;
                        if (!singleton.adIsLoaded) return;
                        singleton.layout.addView(singleton.adView, singleton.adLayoutParams);
                        singleton.adInLayout = true;
                    } else {
                        if (!singleton.adInLayout) return;
                        singleton.layout.removeView(singleton.adView);
                        singleton.adInLayout = false;
                    }
                }
            }
        });
    }

    public static int cppGetAdHeight() {
        Log.i("AMULET", "called cppGetAdHeight");
        synchronized (singleton.adLock) {
            if (singleton.adView == null) return 0;
            return singleton.adView.getHeight();
        }
    }

    public static int cppIsAdVisible() {
        //Log.i("AMULET", "called cppIsAdVisible");
        synchronized (singleton.adLock) {
            if (singleton.adInLayout && singleton.adIsLoaded) {
                return 1;
            } else {
                return 0;
            }
        }
    }

    public static void cppRefreshAd() {
        Log.i("AMULET", "called cppRefreshAd");
        singleton.runOnUiThread(new Runnable() {
            public void run() {
                synchronized (singleton.adLock) {
                    singleton.adIsLoaded = false;
                    AdRequest adRequest = new AdRequest.Builder().build();
                    singleton.adView.loadAd(adRequest);
                }
            }
        });
    }
    */

    // --- IAP -----------------------

    //BILLING private void handleTransactionUpdated(final String productId, final String status, final String token) {
    //BILLING     Log.i("AMULET", "calling handleTransactionUpdated (" + productId + "  " + status + ")");
    //BILLING     view.queueEvent(new Runnable() {
    //BILLING         public void run() {
    //BILLING             Log.i("AMULET", "calling jniIAPTransactionUpdated");
    //BILLING             int res = jniIAPTransactionUpdated(productId, status);
    //BILLING             Log.i("AMULET", "done calling jniIAPTransactionUpdated (res = " + res + ")");
    //BILLING             if (billingClient == null) {
    //BILLING                 Log.i("AMULET", "billingClient is null");
    //BILLING                 return;
    //BILLING             }
    //BILLING             if (token != null) {
    //BILLING                 switch (res) {
    //BILLING                     case 1: {
    //BILLING                             AcknowledgePurchaseParams acknowledgePurchaseParams =
    //BILLING                                 AcknowledgePurchaseParams.newBuilder()
    //BILLING                                     .setPurchaseToken(token)
    //BILLING                                     .build();
    //BILLING                             billingClient.acknowledgePurchase(acknowledgePurchaseParams, new AcknowledgePurchaseResponseListener() {
    //BILLING                                 public void onAcknowledgePurchaseResponse(BillingResult billingResult) {
    //BILLING                                     Log.i("AMULET", "purchase acknowledged (" + billingResult.getDebugMessage() + ")");
    //BILLING                                 }
    //BILLING                             });
    //BILLING                             break;
    //BILLING                         }
    //BILLING                     case 2: {
    //BILLING                         ConsumeParams consumeParams =
    //BILLING                             ConsumeParams.newBuilder()
    //BILLING                                 .setPurchaseToken(token)
    //BILLING                                 .build();
    //BILLING                         billingClient.consumeAsync(consumeParams, new ConsumeResponseListener() {
    //BILLING                             public void onConsumeResponse(BillingResult billingResult, String purchaseToken) {
    //BILLING                                 Log.i("AMULET", "purchase " + purchaseToken + " consumed (" + billingResult.getDebugMessage() + ")");
    //BILLING                             }
    //BILLING                         });
    //BILLING                         break;
    //BILLING                     }
    //BILLING                 }
    //BILLING             }
    //BILLING         }
    //BILLING     });
    //BILLING         
    //BILLING }

    //BILLING public void onPurchasesUpdated(BillingResult billingResult, List<Purchase> purchases) {
    //BILLING     Log.i("AMULET", "onPurchasesUpdated: " + billingResult.getResponseCode() + " " + billingResult.getDebugMessage());
    //BILLING     if (purchases != null && purchases.size() > 0) {
    //BILLING         for (Purchase p : purchases) {
    //BILLING             Log.i("AMULET", "purchase: " + p.getSku());
    //BILLING             String status;
    //BILLING             switch (billingResult.getResponseCode()) {
    //BILLING                 case BillingClient.BillingResponseCode.OK: {
    //BILLING                     switch (p.getPurchaseState()) {
    //BILLING                         case Purchase.PurchaseState.PURCHASED: status = "purchased"; break;
    //BILLING                         case Purchase.PurchaseState.PENDING: status = "deferred"; break;
    //BILLING                         default: status = "unknown"; break;
    //BILLING                     }
    //BILLING                     break;
    //BILLING                 }
    //BILLING                 case BillingClient.BillingResponseCode.ITEM_ALREADY_OWNED: {
    //BILLING                     // Item was not consumed or app doesn't think it's owned.
    //BILLING                     // Either way let's tell the app it's a new purchase which will update the app state or re-consume it.
    //BILLING                     status = "purchased";
    //BILLING                     break;
    //BILLING                 }
    //BILLING                 case BillingClient.BillingResponseCode.USER_CANCELED:
    //BILLING                     status = "cancelled";
    //BILLING                     break;
    //BILLING                 default:
    //BILLING                     status = "failed";
    //BILLING                     break;
    //BILLING             }
    //BILLING             handleTransactionUpdated(p.getSku(), status, p.getPurchaseToken());
    //BILLING         }
    //BILLING         lastPurchaseProductId = null;
    //BILLING     } else if (lastPurchaseProductId != null) {
    //BILLING         String status = "failed";
    //BILLING         switch (billingResult.getResponseCode()) {
    //BILLING             case BillingClient.BillingResponseCode.USER_CANCELED:
    //BILLING                 status = "cancelled";
    //BILLING                 break;
    //BILLING             default:
    //BILLING                 status = "failed";
    //BILLING                 break;
    //BILLING         }
    //BILLING         handleTransactionUpdated(lastPurchaseProductId, status, null);
    //BILLING         lastPurchaseProductId = null;
    //BILLING     }
    //BILLING }

    //BILLING void getProducts(String[] productIds) {
    //BILLING     int success = 0;
    //BILLING     String[] prodIds = null;
    //BILLING     String[] prices = null;
    //BILLING     List<String> skuList = new ArrayList<String> (Arrays.asList(productIds));
    //BILLING     SkuDetailsParams.Builder params = SkuDetailsParams.newBuilder();
    //BILLING     params.setSkusList(skuList).setType(BillingClient.SkuType.INAPP);
    //BILLING     try {
    //BILLING         // wait for connect
    //BILLING         int i = 100;
    //BILLING         while (!billingConnected && i > 0) {
    //BILLING             Thread.sleep(100);
    //BILLING             i--;
    //BILLING         }
    //BILLING         if (!billingConnected) return;
    //BILLING         billingClient.querySkuDetailsAsync(params.build(),
    //BILLING             new SkuDetailsResponseListener() {
    //BILLING                 @Override
    //BILLING                 public void onSkuDetailsResponse(BillingResult billingResult,
    //BILLING                         List<SkuDetails> skuDetailsList) {
    //BILLING                     Log.i("AMULET", "onSkuDetailsResponse msg: " + billingResult.getDebugMessage());
    //BILLING                     if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK) {
    //BILLING                         billingSkuDetails = skuDetailsList;
    //BILLING                         final String[] prodIds = new String[skuDetailsList.size()];
    //BILLING                         final String[] prices = new String[skuDetailsList.size()];
    //BILLING                         int j = 0;
    //BILLING                         for (SkuDetails details : skuDetailsList) {
    //BILLING                             prodIds[j] = details.getSku();
    //BILLING                             prices[j] = details.getPrice();
    //BILLING                             Log.i("AMULET", "got sku: " + details.getSku() + "(" + details.getPrice() + ")");
    //BILLING                             j++;
    //BILLING                         }
    //BILLING                         view.queueEvent(new Runnable() {
    //BILLING                             public void run() {
    //BILLING                                 Log.i("AMULET", "calling jniIAPProductsRetrieved");
    //BILLING                                 jniIAPProductsRetrieved(1, prodIds, prices);
    //BILLING                             }
    //BILLING                         });
    //BILLING                     }
    //BILLING                 }
    //BILLING             }
    //BILLING         );
    //BILLING     } catch (Exception e) {
    //BILLING         Log.i("AMULET", e.getMessage() + ":" + e.getStackTrace());
    //BILLING     }
    //BILLING }

    //BILLING void purchaseProduct(final String productId) {
    //BILLING     if (!billingConnected) return;
    //BILLING     if (billingSkuDetails == null) return;
    //BILLING     SkuDetails details = null;
    //BILLING     for (SkuDetails d : billingSkuDetails) {
    //BILLING         if (d.getSku().equals(productId)) {
    //BILLING             details = d;
    //BILLING             break;
    //BILLING         }
    //BILLING     }
    //BILLING     if (details == null) return;
    //BILLING     try {
    //BILLING         lastPurchaseProductId = productId;
    //BILLING         BillingFlowParams flowParams = BillingFlowParams.newBuilder()
    //BILLING             .setSkuDetails(details)
    //BILLING             .build();
    //BILLING         billingClient.launchBillingFlow(this, flowParams);
    //BILLING         // ignore result of above, because it is delivered via the PurchasesUpdatedListener (docs not clear on this though)
    //BILLING     } catch (Exception e) {
    //BILLING         Log.i("AMULET", e.getMessage() + ":" + e.getStackTrace());
    //BILLING         lastPurchaseProductId = null;
    //BILLING         handleTransactionUpdated(productId, "failed", null);
    //BILLING     }
    //BILLING }

    //BILLING void restorePurchases() {
    //BILLING     try {
    //BILLING         if (!billingConnected) {
    //BILLING             view.queueEvent(new Runnable() {
    //BILLING                 public void run() {
    //BILLING                     jniIAPRestoreFinished(0);
    //BILLING                 }
    //BILLING             });
    //BILLING         }
    //BILLING         Purchase.PurchasesResult pRes = billingClient.queryPurchases(BillingClient.SkuType.INAPP);
    //BILLING         List<Purchase> purchases = pRes.getPurchasesList();
    //BILLING         if (purchases != null) {
    //BILLING             for (Purchase p : purchases) {
    //BILLING                 if (p.getPurchaseState() == Purchase.PurchaseState.PURCHASED) {
    //BILLING                     handleTransactionUpdated(p.getSku(), "restored", p.isAcknowledged() ? null : p.getPurchaseToken());
    //BILLING                 }
    //BILLING             }
    //BILLING             view.queueEvent(new Runnable() {
    //BILLING                 public void run() {
    //BILLING                     jniIAPRestoreFinished(1);
    //BILLING                 }
    //BILLING             });
    //BILLING         } else {
    //BILLING             view.queueEvent(new Runnable() {
    //BILLING                 public void run() {
    //BILLING                     jniIAPRestoreFinished(0);
    //BILLING                 }
    //BILLING             });
    //BILLING         }
    //BILLING     } catch (Exception e) {
    //BILLING         Log.i("AMULET", e.getMessage() + ":" + e.getStackTrace());
    //BILLING         jniIAPRestoreFinished(0);
    //BILLING         view.queueEvent(new Runnable() {
    //BILLING             public void run() {
    //BILLING                 jniIAPRestoreFinished(0);
    //BILLING             }
    //BILLING         });
    //BILLING     }
    //BILLING }

    //BILLING public static void cppGetProducts(String[] productIds0) {
    //BILLING     Log.i("AMULET", "cppGetProducts called");
    //BILLING     final String[] productIds = productIds0;
    //BILLING     new Thread(new Runnable() {
    //BILLING         public void run() {
    //BILLING             singleton.getProducts(productIds);
    //BILLING         }
    //BILLING     }).start();
    //BILLING }

    //BILLING public static void cppRestorePurchases() {
    //BILLING     Log.i("AMULET", "cppRestorePurchases called");
    //BILLING     singleton.restorePurchases();
    //BILLING }

    //BILLING public static void cppPurchaseProduct(String productId) {
    //BILLING     Log.i("AMULET", "cppPurchaseProduct called");
    //BILLING     singleton.purchaseProduct(productId);
    //BILLING }
    

    // -- Game services API ----------

    static void cppInitGoogleApiClient() {
        //  for leaderboards, achievements
        singleton.runOnUiThread(new Runnable() {
            public void run() {
                singleton.googleApiClient = new GoogleApiClient.Builder(singleton)
                        .addConnectionCallbacks(singleton)
                        .addOnConnectionFailedListener(singleton)
                        .addApi(Games.API).addScope(Games.SCOPE_GAMES)
                        .build();
                singleton.googleApiClient.connect();
            }
        });
    }

    @Override
    public void onConnected(Bundle bundle) {
        Log.i("AMULET", "Google services connected!");
        view.queueEvent(new Runnable() {
            public void run() {
                Log.i("AMULET", "calling jniSetGameServicesConnected (1)");
                jniSetGameServicesConnected(1);
            }
        });
    }

    @Override
    public void onConnectionSuspended(int i) {
        Log.i("AMULET", "Google services suspended!");
        view.queueEvent(new Runnable() {
            public void run() {
                Log.i("AMULET", "calling jniSetGameServicesConnected (0)");
                jniSetGameServicesConnected(0);
            }
        });
    }

    @Override
    public void onConnectionFailed(ConnectionResult connectionResult)  {
        Log.i("AMULET", "Google services connection failed! "+connectionResult.getErrorMessage() + " " + connectionResult.getErrorCode());
        view.queueEvent(new Runnable() {
            public void run() {
                Log.i("AMULET", "calling jniSetGameServicesConnected (0)");
                jniSetGameServicesConnected(0);
            }
        });
        if (googleApiClientConnectionAttempts == 0) {
            Log.i("AMULET", "Google services trying reconnect");
            // on my device it always fails to connect on first attempt after
            // installation and then works after that, so try again once.
            googleApiClientConnectionAttempts++;
            cppInitGoogleApiClient();
        }
    }

    public static void cppSubmitLeaderboardScore(String leaderboard0, long score0) {
        final String leaderboard = leaderboard0;
        final long score = score0;
        singleton.runOnUiThread(new Runnable() {
            public void run() {
                if(singleton.googleApiClient != null && singleton.googleApiClient.isConnected()) {
                    Games.Leaderboards.submitScore(singleton.googleApiClient, leaderboard, score);
                }
            }
        });
    }

    public static void cppSubmitAchievement(String achievement0) {
        final String achievement = achievement0;
        singleton.runOnUiThread(new Runnable() {
            public void run() {
                if(singleton.googleApiClient != null && singleton.googleApiClient.isConnected()) {
                    //Log.i("AMULET", "unlock " + achievement);
                    Games.Achievements.unlock(singleton.googleApiClient, achievement);
                }
            }
        });
    }

    public static void cppShowLeaderboard(String leaderboard0) {
        final String leaderboard = leaderboard0;
        singleton.runOnUiThread(new Runnable() {
            public void run() {
                if(singleton.googleApiClient != null && singleton.googleApiClient.isConnected()) {
                    singleton.startActivityForResult(Games.Leaderboards.getLeaderboardIntent(singleton.googleApiClient,
                            leaderboard), 1002);
                }
            }
        });
    }

    // --- JNI ------------

    static {
        System.loadLibrary("amulet");
    }

    public static native void jniResize(int width, int height);
    public static native void jniStep();
    public static native void jniInit(AssetManager assman, String datadir, String lang);
    public static native void jniSurfaceCreated();
    public static native void jniTeardown();
    public static native void jniPause();
    public static native void jniResume();
    public static native void jniTouchDown(int id, float x, float y);
    public static native void jniTouchUp(int id, float x, float y);
    public static native void jniTouchMove(int id, float x, float y);
    public static native void jniFillAudioBuffer(float[] buf, int size);
    public static native void jniIAPProductsRetrieved(int success, String[] productids, String[] prices);
    public static native int jniIAPTransactionUpdated(String productId, String status);
    public static native void jniIAPRestoreFinished(int success);
    public static native void jniSetGameServicesConnected(int flag);
}
