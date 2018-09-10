package xyz.amulet;

import android.app.PendingIntent;
import android.app.Activity;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
import android.view.WindowManager;
import android.content.*;
import android.content.res.*;

import android.opengl.GLSurfaceView;
import android.view.*;

import android.graphics.Color;
import android.widget.*;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.opengles.GL10;

import java.util.*;
import org.json.*;

import com.google.android.gms.ads.*;
import com.android.vending.billing.*;
import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.api.GoogleApiClient;
import com.google.android.gms.games.Games;

public class AmuletActivity extends Activity
        implements
        GoogleApiClient.ConnectionCallbacks,
        GoogleApiClient.OnConnectionFailedListener {
    AmuletView view = null;
    AdView adView = null;
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

    public AmuletActivity() {
        singleton = this;
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

        // iap stuff
        Intent serviceIntent =
                new Intent("com.android.vending.billing.InAppBillingService.BIND");
        serviceIntent.setPackage("com.android.vending");
        bindService(serviceIntent, iapServiceCon, Context.BIND_AUTO_CREATE);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (iapService != null) {
            unbindService(iapServiceCon);
        }
        jniTeardown();
    }

    @Override protected void onPause() {
        super.onPause();
        view.onPause();
        jniPause();
    }

    @Override protected void onResume() {
        super.onResume();
        view.onResume();
        jniResume();
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

    // --- IAP -----------------------

    IInAppBillingService iapService;

    ServiceConnection iapServiceCon = new ServiceConnection() {
        @Override
        public void onServiceDisconnected(ComponentName name) {
            iapService = null;
        }

        @Override
        public void onServiceConnected(ComponentName name,
                                       IBinder service) {
            iapService = IInAppBillingService.Stub.asInterface(service);
            //getProducts();
        }
    };

    void getProducts(String[] productIds) {
        int success = 0;
        String[] prodIds = null;
        String[] prices = null;
        ArrayList<String> skuList = new ArrayList<String> (Arrays.asList(productIds));
        Bundle querySkus = new Bundle();
        querySkus.putStringArrayList("ITEM_ID_LIST", skuList);
        try {
            // wait for connect
            int i = 100;
            while (iapService == null && i > 0) {
                Thread.sleep(100);
                i--;
            }
            if (iapService == null) return;
            Bundle skuDetails = iapService.getSkuDetails(3,
                    getPackageName(), "inapp", querySkus);
            int response = skuDetails.getInt("RESPONSE_CODE");
            if (response == 0) {
                ArrayList<String> responseList
                        = skuDetails.getStringArrayList("DETAILS_LIST");

                prodIds = new String[responseList.size()];
                prices = new String[responseList.size()];
                int ii = 0;
                for (String thisResponse : responseList) {
                    JSONObject object = new JSONObject(thisResponse);
                    prodIds[ii] = object.getString("productId");
                    prices[ii] = object.getString("price");
                    ii++;
                    Log.i("AMULET", thisResponse);
                }

                success = 1;
                //purchaseProduct("android.test.purchased");
                //purchaseProduct("android.test.canceled");
            } else {
                Log.i("AMULET", "non-zero getproducts response: " + response);
            }
        } catch (Exception e) {
            Log.i("AMULET", e.getMessage() + ":" + e.getStackTrace());
        }
        final int fsuccess = success;
        if (success == 0) {
            prodIds = new String[0];
            prices = new String[0];
        }
        final String[] fprodIds = prodIds;
        final String[] fprices = prices;
        view.queueEvent(new Runnable() {
            public void run() {
                Log.i("AMULET", "calling jniIAPProductsRetrieved");
                jniIAPProductsRetrieved(fsuccess, fprodIds, fprices);
            }
        });
    }

    void purchaseProduct(final String productId) {
        try {
            synchronized (pendingProductIdLock) {
                if (pendingProductId != null) {
                    view.queueEvent(new Runnable() {
                        public void run() {
                            Log.i("AMULET", "calling jniIAPTransactionUpdated (failed - multiple purchases)");
                            jniIAPTransactionUpdated(productId, "failed");
                        }
                    });
                    return;
                }
                Bundle buyIntentBundle = iapService.getBuyIntent(3, getPackageName(), productId, "inapp", "");
                int response = buyIntentBundle.getInt("RESPONSE_CODE");
                if (response == 0) {
                    PendingIntent pendingIntent = buyIntentBundle.getParcelable("BUY_INTENT");
                    startIntentSenderForResult(pendingIntent.getIntentSender(),
                            1001, new Intent(), Integer.valueOf(0), Integer.valueOf(0),
                            Integer.valueOf(0));
                    pendingProductId = productId;
                } else {
                    Log.i("AMULET", "non-zero getbuyintent response: " + response);
                    view.queueEvent(new Runnable() {
                        public void run() {
                            Log.i("AMULET", "calling jniIAPTransactionUpdated (failed - non-zero response)");
                            jniIAPTransactionUpdated(productId, "failed");
                        }
                    });
                }
            }
        } catch (Exception e) {
            Log.i("AMULET", e.getMessage() + ":" + e.getStackTrace());
            view.queueEvent(new Runnable() {
                public void run() {
                    Log.i("AMULET", "calling jniIAPTransactionUpdated (failed - exception)");
                    jniIAPTransactionUpdated(productId, "failed");
                }
            });
        }
    }

    void restorePurchases() {
        try {
            Bundle ownedItems = iapService.getPurchases(3, getPackageName(), "inapp", null);
            int response = ownedItems.getInt("RESPONSE_CODE");
            if (response == 0) {
                final ArrayList<String> ownedProductIds = ownedItems.getStringArrayList("INAPP_PURCHASE_ITEM_LIST");
                view.queueEvent(new Runnable() {
                    public void run() {
                        for (String productId : ownedProductIds) {
                            jniIAPTransactionUpdated(productId, "restored");
                        }
                        jniIAPRestoreFinished(1);
                    }
                });
            } else {
                Log.i("AMULET", "invalid restore purchases response: " + response);
                view.queueEvent(new Runnable() {
                    public void run() {
                        jniIAPRestoreFinished(0);
                    }
                });
            }
        } catch (Exception e) {
            Log.i("AMULET", e.getMessage() + ":" + e.getStackTrace());
            view.queueEvent(new Runnable() {
                public void run() {
                    jniIAPRestoreFinished(0);
                }
            });
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == 1001) {
            synchronized (pendingProductIdLock) {
                if (pendingProductId == null) {
                    return;
                }
                final String productId = pendingProductId;
                pendingProductId = null;

                Log.i("AMULET", "onActivityResult 1001: " + resultCode);
                //int responseCode = data.getIntExtra("RESPONSE_CODE", 0); // XXX what is this for?
                //String purchaseData = data.getStringExtra("INAPP_PURCHASE_DATA");
                //String dataSignature = data.getStringExtra("INAPP_DATA_SIGNATURE");

                final String status;
                switch (resultCode) {
                    case RESULT_OK: status = "purchased"; break;
                    case RESULT_CANCELED: status = "cancelled"; break;
                    default: status = "failed"; break;
                }
                view.queueEvent(new Runnable() {
                    public void run() {
                        Log.i("AMULET", "calling jniIAPTransactionUpdated (" + status + ")");
                        jniIAPTransactionUpdated(productId, status);
                    }
                });
            }
        }
    }

    public static void cppGetProducts(String[] productIds0) {
        Log.i("AMULET", "cppGetProducts called");
        final String[] productIds = productIds0;
        new Thread(new Runnable() {
            public void run() {
                singleton.getProducts(productIds);
            }
        }).start();
    }

    public static void cppRestorePurchases() {
        Log.i("AMULET", "cppRestorePurchases called");
        new Thread(new Runnable() {
            public void run() {
                singleton.restorePurchases();
            }
        }).start();
    }


    public static void cppPurchaseProduct(String productId0) {
        Log.i("AMULET", "cppPurchaseProduct called");
        final String productId = productId0;
        new Thread(new Runnable() {
            public void run() {
                singleton.purchaseProduct(productId);
            }
        }).start();
    }

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
        Log.i("AMULET", "Google services connection failed!");
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
    public static native void jniIAPProductsRetrieved(int success, String[] productids, String[] prices);
    public static native void jniIAPTransactionUpdated(String productId, String status);
    public static native void jniIAPRestoreFinished(int success);
    public static native void jniSetGameServicesConnected(int flag);
}
