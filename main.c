#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <math.h>

#define AddEnemy(a,b) ObjectInit(NewObject(), a,b,40,40,'e')

RECT rct;
int counter = 0;

typedef struct Spoint{
    float x, y;
} TPoint;

TPoint point(float x, float y){
    TPoint pt;
    pt.x = x;
    pt.y = y;
    return pt;
}

TPoint cam;

typedef struct SObject
{
    TPoint pos;
    TPoint size;
    COLORREF brush;
    TPoint speed;
    char oType;
    float range, vecSpeed;
    BOOL isDel;
} TObject, *PObject;

BOOL ObjectCollision(TObject o1, TObject o2){
    return ((o1.pos.x + o1.size.x) > o2.pos.x) && (o1.pos.x < (o2.pos.x + o2.size.x)) &&
           ((o1.pos.y + o1.size.y) > o2.pos.y) && (o1.pos.y < (o2.pos.y + o2.size.y));
}

void ObjectInit(TObject *obj, float xPos, float yPos, float width, float height, char objType){
    obj->pos = point(xPos, yPos);
    obj->size = point(width, height);
    obj->brush = RGB(0,255,0);
    obj->speed = point(0,0);
    obj->oType = objType;
    if (objType == 'e') obj->brush = RGB(255, 0,0);
    obj->isDel = FALSE;
}

void ObjectShow(TObject obj, HDC dc){
    SelectObject(dc, GetStockObject(DC_PEN));
    SetDCPenColor(dc, RGB(0,0,0));
    SelectObject(dc, GetStockObject(DC_BRUSH));
    SetDCBrushColor(dc, obj.brush);

    BOOL (*shape)(HDC, int, int,int,int);
    shape = obj.oType == 'e' ? Ellipse : Rectangle;
    shape(dc, (int)(obj.pos.x - cam.x), (int)(obj.pos.y - cam.y),
          (int)(obj.pos.x + obj.size.x - cam.x), (int)(obj.pos.y + obj.size.y - cam.y));

}

void ObjectSetDestPoint(TObject *obj, float xPos, float yPos, float vecSpeed){
    TPoint xyLen = point(xPos - obj->pos.x, yPos - obj->pos.y);
    float dxy = sqrt(xyLen.x * xyLen.x + xyLen.y * xyLen.y );
    obj->speed.x = xyLen.x / dxy *vecSpeed;
    obj->speed.y = xyLen.y / dxy *vecSpeed;
    obj->vecSpeed = vecSpeed;

}

TObject player;

PObject mass = NULL;
int masCnt = 0;

BOOL needNewGame = FALSE;

void SetCameraFocus(TObject obj){
    cam.x = obj.pos.x - rct.right / 2;
    cam.y = obj.pos.y - rct.bottom / 2;
}

void ObjectMove(TObject *obj){
    if (obj->oType == 'e'){
        if (rand() % 40 == 1){
            static float enemySpeed = 1.5;
            ObjectSetDestPoint(obj, player.pos.x, player.pos.y, enemySpeed);
        }
        if (ObjectCollision(*obj, player))
            needNewGame = TRUE;
    }

    obj->pos.x += obj->speed.x;
    obj->pos.y += obj->speed.y;

    if (obj->oType == '1')
    {
        obj->range -= obj->vecSpeed;
        if (obj->range < 0){
            obj->isDel = TRUE;
            counter++;
        }


        for (int i = 0; i < masCnt; i++)
            if ((mass[i].oType == 'e') && (ObjectCollision(*obj, mass[i]))){
                mass[i].isDel = TRUE;
                obj->isDel = TRUE;
            }
    }
}

PObject NewObject(){
    masCnt++;
    mass = realloc(mass, sizeof(*mass) * masCnt);
    return mass + masCnt - 1;
}
void GenNewEnemy(){
    static int rad = 300;
    int pos1 = (rand() % 2 == 0 ? -rad : rad);
    int pos2 = (rand() % (rad * 2)) - rad;
    int k = rand() % 100;
    if (k==1)
        AddEnemy(player.pos.x + pos1, player.pos.y + pos2);
    if (k == 2)
        AddEnemy(player.pos.x + pos2, player.pos.y + pos1);

}

void DelObjects(){
    int i = 0;
    while (i < masCnt){
        if (mass[i].isDel){
            masCnt--;
            mass[i] = mass[masCnt];
            mass = realloc(mass, sizeof(*mass) * masCnt);
        }
        else
            i++;
    }
}

void AddBullet(float xPos, float yPos, float x, float y){
    PObject obj = NewObject();
    ObjectInit(obj, xPos, yPos, 10, 10, '1');
    ObjectSetDestPoint(obj, x, y, 5);
    obj->range = 300;
}

void PlayerControl(){
    static int playerSpeed = 2;
    player.speed.x = 0;
    player.speed.y = 0;
    if (GetKeyState('W') < 0) player.speed.y = -playerSpeed;
    if (GetKeyState('A') < 0) player.speed.x = -playerSpeed;
    if (GetKeyState('S') < 0) player.speed.y = playerSpeed;
    if (GetKeyState('D') < 0) player.speed.x = playerSpeed;
    if ((player.speed.x != 0) && (player.speed.y != 0))
        player.speed = point(player.speed.x * 0.7, player.speed.y = 0.7);

}

void WinInit(){
    needNewGame = FALSE;
    masCnt = 0;
    mass  = realloc(mass, 0);
    ObjectInit(&player, 100,100,40,40, 'p');
}

void WinMove(){
    if (needNewGame){
        printf("Scores: %d\n", counter);
        counter = 0;
        WinInit();
    }


    PlayerControl();
    ObjectMove(&player);
    SetCameraFocus(player);

    for (int i=0;i<masCnt; i++)
        ObjectMove(mass + i);

    GenNewEnemy();
    DelObjects();
}

void WinShow(HDC dc){
    HDC memDC = CreateCompatibleDC(dc);
    HBITMAP memBM = CreateCompatibleBitmap(dc, rct.right - rct.left, rct.bottom - rct.top);
    SelectObject(memDC, memBM);

    SelectObject(memDC, GetStockObject(DC_PEN));
    SetDCPenColor(memDC, RGB(255,255,255));
    SelectObject(memDC, GetStockObject(DC_BRUSH));
    SetDCBrushColor(memDC, RGB(200,200,200));

    static int rectSize = 200;
    int dx = (int)(cam.x) % rectSize;
    int dy = (int)(cam.y) % rectSize;
    for (int i = -1; i < (rct.right / rectSize) + 2; i++)
        for (int j = -1; j < (rct.bottom / rectSize) + 2; j++)
            Rectangle(memDC, -dx+(i*rectSize),-dy+(j*rectSize),
                      -dx+((i+1)*rectSize),-dy+((j+1)*rectSize));

    ObjectShow(player, memDC);

    for (int i=0; i< masCnt;i++){
        ObjectShow(mass[i],memDC);
    }

    BitBlt(dc, 0,0,rct.right - rct.left, rct.bottom - rct.top, memDC, 0,0, SRCCOPY);
    DeleteDC(memDC);
    DeleteObject(memBM);
}

// обработка сообщений
LRESULT WinProc (HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam){
    if (message == WM_DESTROY)
        PostQuitMessage(0);

    //    else if(message == WM_KEYDOWN)
    //        printf("code = %lld\n", wparam);

    //    else if (message == WM_CHAR)
    //        printf("symb - %lld\n", wparam);

    else if (message == WM_SIZE)
        GetClientRect(hwnd, &rct);

    else if (message == WM_LBUTTONDOWN){
        int xPos = LOWORD(lparam);
        int yPos = HIWORD(lparam);
        AddBullet(player.pos.x + player.size.x / 2,
                  player.pos.y + player.size.y / 2,
                  xPos + cam.x, yPos + cam.y);
    }
    else if (message == WM_MOUSEMOVE){
        int xPos = LOWORD(lparam);
        int yPos = HIWORD(lparam);
        //        printf("mouse [%d][%d]\n", xPos, yPos);
    }

    else return DefWindowProcA(hwnd, message, wparam, lparam);
}
int main()
{
    WNDCLASSA wcl;
    HWND hwnd;  // дескриптор окна
    MSG msg;

    memset(&wcl, 0, sizeof(WNDCLASSA));
    wcl.lpszClassName = "my Window";    // Название класса
    wcl.lpfnWndProc = WinProc;          // Обработка сообщений
    wcl.hCursor = LoadCursorA(NULL, IDC_CROSS);
    RegisterClassA(&wcl);


    hwnd = CreateWindow("my Window", "Window Name", WS_OVERLAPPEDWINDOW,
                        10,10,640,480,NULL,NULL,NULL,NULL);

    ShowWindow(hwnd, SW_SHOWNORMAL);    // SHOWNORMAL для показа окна


    HDC dc = GetDC(hwnd);;     // контекст устройства

    WinInit();
    while(1)
    {
        if (PeekMessageA(&msg, NULL, 0,0,PM_REMOVE)){
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);  // Передача сообщения в окно
        }
        else{
            WinShow(dc);
            WinMove();
            Sleep(0.9);
        }

    }
    printf("This is the end");
    return 0;
}




