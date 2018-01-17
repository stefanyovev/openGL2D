    
    
    #include <windows.h>
    #include <mmsystem.h>
    
    #include <gl/gl.h>
    #define GLUT_DISABLE_ATEXIT_HACK
    #include <gl/glut.h>
    #include <cstdlib> // exit(), rand()
    #include <cstdio>  // malloc(), free()
    #include <cmath>   // sqrt(), abs()

    /*
      texture represents an image file
        it can be drawn on screen
        
      sprite represents a series of regions of a texture
        it has texture, scale, frame width, frame height, column and row
        it can be drawn scaled or flipped on screen
        
      objects represents a material point
        it has position, velocity, acceleration, mass and sprite
        can be forced with force vector and drawn
        
      math.h (cmath) does not have vector class
      c++ stl does have but its more like array imlementation not the math meaning
    */
    class vector { public:
        float x, y;
        inline vector() : x(0), y(0) {};
        vector( float a, float b ) : x(a), y(b) {};
        float size(){ return sqrt( x*x + y*y ); };
        vector operator + ( const vector& b ){ return vector( this->x + b.x, this->y + b.y ); };
        vector operator - ( const vector& b ){ return vector( this->x - b.x, this->y - b.y ); };
        vector operator * ( const float& b ){ return vector( this->x * b, this->y * b ); };
        vector operator / ( const float& b ){ return vector( this->x / b, this->y / b ); };
        void operator += ( const vector& b ){ this->x += b.x; this->y += b.y; }
        void operator -= ( const vector& b ){ this->x -= b.x; this->y -= b.y; }
        void operator *= ( float b ){ this->x *= b; this->y *= b; }
        void operator /= ( float b ){ this->x /= b; this->y /= b; }
    } v0;

    /*
      texture class; the constructor will load 512x512 bgr bitmap
      we need rgba so have to swap every pixel
      m1 is original data; m2 - converted
    */
    class texture { public:
        unsigned int name;
        texture( const char* file ){
            unsigned char *m1 = (unsigned char *) malloc(512*512*3);
            unsigned char *m2 = (unsigned char *) malloc(512*512*4);
            FILE* f = fopen( file, "rb" ); // read binary
            fread( m1, 1, 54, f ); // skip header of the file
            fread( m1, 1, 512*512*3, f ); // read all data
            fclose(f);
            for( int n=0; n<512*512; n++ ){ // foreach pixel
                m2[n*4+0] = m1[n*3+2]; // copy and reform
                m2[n*4+1] = m1[n*3+1];
                m2[n*4+2] = m1[n*3+0];
                // if color is (255,0,255) make the alpha chanel 0 else 255
                m2[n*4+3] = (m1[n*3+0]==255 && m1[n*3+1]==0 && m1[n*3+2]==255)?0:255;
            }
        	glGenTextures( 1, &name );
        	glBindTexture( GL_TEXTURE_2D, name );
    	    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    	    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    	    glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
        	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 512, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, m2 );
        	free(m1);
        	free(m2);
        };
        void draw( float x, float y, float repeatx, float repeaty ){
       	    glBindTexture( GL_TEXTURE_2D, name );
       	    glBegin( GL_POLYGON );
                glTexCoord2f( 0, 0 ); glVertex2f( x, y );
                glTexCoord2f( repeatx, 0 ); glVertex2f( x+512*repeatx, y );
                glTexCoord2f( repeatx, repeaty ); glVertex2f( x+512*repeatx, y+512*repeaty );
                glTexCoord2f( 0, repeaty ); glVertex2f( x, y+512*repeaty );
            glEnd();
        }
    };

    class sprite : public texture { public:
        unsigned char fw, fh, col, row;
        float scale;
        sprite( const char* file, unsigned char fw, unsigned char fh, float scale )
            : texture(file), fw(fw), fh(fh), col(0), row(0), scale(scale) {};
        void setrow( unsigned char x ){ row=x; col=0; };
        void draw( float x, float y, bool flip = false ){
            // draw current part of the texture on the correct place onscreen
            float tw=((float)fw)/512.0, th=((float)fh)/512.0;
       	    glBindTexture( GL_TEXTURE_2D, name );
            glBegin( GL_POLYGON );
                glTexCoord2f( tw*col, 1-th-th*row ); glVertex2f( flip ? x+fw*scale : x, y );
                glTexCoord2f( tw*col+tw, 1-th-th*row ); glVertex2f( flip ? x : x+fw*scale, y );
                glTexCoord2f( tw*col+tw, 1-th*row ); glVertex2f( flip ? x : x+fw*scale, y+fh*scale );
                glTexCoord2f( tw*col, 1-th*row ); glVertex2f( flip ? x+fw*scale : x, y+fh*scale );
            glEnd();
        };
    };

    class object : public sprite { public:
        vector p,v,a; float m;
        // for an object we have to construct texture and sprite with 1 constructor
        object( const char* file, unsigned char fw, unsigned char fh, float scale, float x, float y, float mass )
            : sprite(file,fw,fh,scale), p(vector(x,y)), v(v0), a(v0), m(mass) {}
        void force( vector f ){ a += f/m; } // hit this object
        void damp( float q ){ v /= q; } // we have friction, cant move constantly
        void update(){ v += a; p += v; a = v0; } // acumulate into position
        void draw(){ sprite::draw( p.x-((float)fw)/2.0, p.y, v.x<0 ); } // now we know where to draw this
    };

    // -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

    #define TITLE "Girl's enemy" // window title
    int width=1200, height=612; // window dim
    
    int t=0; // time in miliseconds
    int ku=0, kd=0, kl=0, kr=0; // arrow keys state
    vector R=vector(15,0), L=vector(-15,0), U=vector(0,15), D=vector(0,-15); // common forces in game
    texture *sky, *ground, *tree;
    object *p, *e, *bs[4]; // player, enemy and birds
    bool gameover = false;

    void initgame(){
        PlaySound( (LPCSTR) "soundtrack.wav", NULL, SND_FILENAME | SND_ASYNC | SND_LOOP );
        sky = new texture( "sky.bmp" );
        ground = new texture( "ground.bmp" );
        tree = new texture( "tree.bmp" );
        p = new object( "player.bmp", 30, 56, 1, 100, 0, 45 );
        e = new object( "enemy.bmp", 128, 128, 1, width-100, 0, 100 );
        // generate 4 birds with random position on screen
        for(int i=0;i<4;i++) bs[i] = new object( "bird.bmp", 128, 128, 0.2, rand()%width, rand()%(height-200)+100, 5 );
    }

    void tick( int ){ t += 20; // GAME MAIN {
        
        // player
        p->damp( 1.05 );
        if( kr && p->p.y<1 ) p->force( R );
        if( kl && p->p.y<1 ) p->force( L );
        if( ku && p->p.y<1 ) p->force( (U+p->a*4)*70 );
        p->force( D );
        if( kd ) p->a *= 2;
        p->update();
        if( p->p.y<0 ) p->p.y=0;
        if( p->p.x<0 ) p->p.x=0;
        if( p->p.x>width ) p->p.x=width;
        if( t % 100 == 0 ){ // determine next frame of player sprite every 100 miliseconds
            switch( p->row ){
                case 0: // avatar idle anim
                    if( abs(p->v.x) > 0.5 ) p->setrow(1);
                    if( abs(p->p.y) > 1 ) p->setrow(2);
                    break;
                case 1: // walk anim is on
                    if( p->p.y > 0 ) p->setrow(2);
                    if ( abs(p->v.x) < 0.5 ) p->setrow(0);
                    break;
                case 2: // jump adnim is on
                    if( p->p.y == 0 ) p->setrow(1);
            };
            if( !(p->row==2&&p->col==3) ) p->col++; // goto next frame
            if( (p->row==0&&p->col==4)||(p->row==1&&p->col==6)||(p->row==2&&p->col==4) ) p->col = 0; // endrow ?
        }
        
        if( !gameover ){
            
            //enemy
            e->damp( 1.05 );
            float dist1 = abs(p->p.x-e->p.x), dist2=width-dist1;
            if( dist1<dist2 ) e->force( e->p.x-p->p.x<0 ? R : L );
            else e->force( e->p.x-p->p.x<0 ? L : R );
            e->update();
            if( e->p.x < 0 ) e->p.x = width;
            if( e->p.x > width ) e->p.x = 0;
            if( t % 100 == 0){
                e->col++;
                if( e->col==4 ){ e->row++; e->col=0; };
                if( e->row==2 ){ e->row = e->col = 0; };
            }   

            // birds
            for(int i=0;i<4;i++){ // foreach
                bs[i]->damp( 1.05 );
                bs[i]->force( vector( rand()%2==0?1:-1, rand()%2==0?1:-1 ) ); // hit with random force
                bs[i]->update();
                if( bs[i]->p.y<150 ) bs[i]->p.y=150; // dont fly low
                if( bs[i]->p.y>height-100 ) bs[i]->p.y=height-100; // dont fly high
                if( bs[i]->p.x<0 ) bs[i]->p.x=0; // dont go out left
                if( bs[i]->p.x>width ) bs[i]->p.x=width; // ..
                bs[i]->col++; //  next frame
                if( bs[i]->col==4 ){ bs[i]->row++; bs[i]->col=0; }; // end of sprite sheet row ?
                if( bs[i]->row==4 ){ bs[i]->row = bs[i]->col = 0; }; // end spritesheet?
            }
        }    

        // gameover ?
        float dx=abs(e->p.x-p->p.x), dy=abs(e->p.y-p->p.y);
        if( dy < e->fh * e->scale && dx < e->fw * e->scale/3 + p->fw * p->scale/3 ){
            PlaySound( NULL, NULL, 0 );
            gameover = true;
        }
        
        // } GAME MAIN;
                
        glutPostRedisplay();
        glutTimerFunc( 20, tick, 1 );
    }

    void draw() {
        glClear( GL_COLOR_BUFFER_BIT );
        glLoadIdentity();
        glTranslatef(0,100,0);
        
        sky->draw( 0, 0, 10, 1 );
        tree->draw( 400 +sin(t/100), 0, 1, 1 ); // x position + sin(time)
        ground->draw( 0, -512, 10, 1 );
        for(int i=0;i<4;i++) bs[i]->draw();
        
        p->draw();
        e->draw();

        glutSwapBuffers();
    }
    
    // -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

    void skeydown( int key, int x, int y) { // On Special key down
        switch ( key ) {
    		case GLUT_KEY_UP: ku = 1; break;
    		case GLUT_KEY_DOWN: kd = 1; break;
    		case GLUT_KEY_LEFT: kl = 1; break;
    		case GLUT_KEY_RIGHT: kr = 1; break;
        }
    }
    
    void skeyup( int key, int x, int y) {
        switch ( key ) {
    		case GLUT_KEY_UP: ku = 0; break;
    		case GLUT_KEY_DOWN: kd = 0; break;
    		case GLUT_KEY_LEFT: kl = 0; break;
    		case GLUT_KEY_RIGHT: kr = 0; break;
        }
    }
     
    void keydown( unsigned char key, int x, int y ){ // on normal key down
        if( key == 27 ) exit(0); // exit with ESC
    }

    static void resize(int w, int h){ // Called when user resizes the window
        width = w;
        height = h;
        glMatrixMode( GL_PROJECTION );
        glLoadIdentity();
        glViewport(0, 0, w, h);
        glOrtho( 0, w, 0, h, 0, 1 );
    	glMatrixMode( GL_MODELVIEW );
    }
        
    int main( int argc, char** argv ) {
        glutInit( &argc, argv );
        glutInitDisplayMode( GLUT_DOUBLE );
        glutInitWindowSize( width, height );
        glutInitWindowPosition( (glutGet(GLUT_SCREEN_WIDTH)-width)/2, (glutGet(GLUT_SCREEN_HEIGHT)-height)/2 );
        glutCreateWindow( TITLE );
        glutDisplayFunc( draw );
        glutKeyboardFunc( keydown );
        glutSpecialFunc( skeydown );
        glutSpecialUpFunc( skeyup );
        glutReshapeFunc( resize );
        glEnable( GL_TEXTURE_2D );
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA ,GL_ONE_MINUS_SRC_ALPHA );
        glShadeModel( GL_SMOOTH );
        initgame();
        glutTimerFunc( 1, tick, 1 );
        glutMainLoop();
        return 0;
    }
