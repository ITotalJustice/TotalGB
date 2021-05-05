// SOURCE: https://github.com/Immediate-Mode-UI/Nuklear/blob/master/demo/sdl_opengl2/nuklear_sdl_gl2.h

#include "gl2.h"
#include "nuklear/defines.h"
#include "nuklear/nuklear.h"

#include <stddef.h> // offsetof
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <GL/gl.h>


struct gl2_device {
    struct nk_buffer cmds;
    struct nk_draw_null_texture null;
    GLuint font_tex;
};

struct gl2_vertex {
    float position[2];
    float uv[2];
    nk_byte col[4];
};

typedef struct gl2_ctx {
    struct gl2_device ogl;
    struct nk_context ctx;
    struct nk_font_atlas atlas;
} ctx_t;

#define PRIVATE_TO_SELF ctx_t* self = (ctx_t*)_private


static void gl2_device_upload_atlas(
	ctx_t* self,
	const void *image,
	int width, int height
) {
    struct gl2_device *dev = &self->ogl;
    glGenTextures(1, &dev->font_tex);
    glBindTexture(GL_TEXTURE_2D, dev->font_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(
    	GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, image
    );
}

static void internal_quit(void* _private) {
	PRIVATE_TO_SELF;

	struct gl2_device *dev = &self->ogl;
    nk_font_atlas_clear(&self->atlas);
    nk_free(&self->ctx);
    glDeleteTextures(1, &dev->font_tex);
    nk_buffer_free(&dev->cmds);

    free(self);
}

static void internal_font_stash_begin(void* _private) {
	PRIVATE_TO_SELF;

	nk_font_atlas_init_default(&self->atlas);
    nk_font_atlas_begin(&self->atlas);
}

static void internal_font_stash_end(void* _private) {
	PRIVATE_TO_SELF;

	int w, h;
    const void* image = nk_font_atlas_bake(&self->atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
    gl2_device_upload_atlas(self, image, w, h);
    nk_font_atlas_end(&self->atlas, nk_handle_id((int)self->ogl.font_tex), &self->ogl.null);
    
    if (self->atlas.default_font) {
        nk_style_set_font(&self->ctx, &self->atlas.default_font->handle);
    }
}

static bool internal_event(void* _private) {
	PRIVATE_TO_SELF;

	struct nk_context *ctx = &self->ctx;

    (void)ctx;
    return 0;
}

static void internal_render(void* _private, enum nk_anti_aliasing AA) {
	PRIVATE_TO_SELF;


	/* setup global state */
    struct gl2_device *dev = &self->ogl;
    int width = 160*2, height = 144*2;
    int display_width = 160*2, display_height = 144*2;
    struct nk_vec2 scale;

    // SDL_GetWindowSize(self->win, &width, &height);
    // SDL_GL_GetDrawableSize(self->win, &display_width, &display_height);
    scale.x = (float)display_width/(float)width;
    scale.y = (float)display_height/(float)height;

    glPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT|GL_TRANSFORM_BIT);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* setup viewport/project */
    glViewport(0,0,(GLsizei)display_width,(GLsizei)display_height);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0f, width, height, 0.0f, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    {
        GLsizei vs = sizeof(struct gl2_vertex);
        size_t vp = offsetof(struct gl2_vertex, position);
        size_t vt = offsetof(struct gl2_vertex, uv);
        size_t vc = offsetof(struct gl2_vertex, col);

        /* convert from command queue into draw list and draw to screen */
        const struct nk_draw_command *cmd;
        const nk_draw_index *offset = NULL;
        struct nk_buffer vbuf, ebuf;

        /* fill converting configuration */
        struct nk_convert_config config;
        static const struct nk_draw_vertex_layout_element vertex_layout[] = {
            {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct gl2_vertex, position)},
            {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct gl2_vertex, uv)},
            {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct gl2_vertex, col)},
            {NK_VERTEX_LAYOUT_END}
        };

        memset(&config, 0, sizeof(config));
        
        config.vertex_layout = vertex_layout;
        config.vertex_size = sizeof(struct gl2_vertex);
        config.vertex_alignment = NK_ALIGNOF(struct gl2_vertex);
        config.null = dev->null;
        config.circle_segment_count = 22;
        config.curve_segment_count = 22;
        config.arc_segment_count = 22;
        config.global_alpha = 1.0f;
        config.shape_AA = AA;
        config.line_AA = AA;

        /* convert shapes into vertexes */
        nk_buffer_init_default(&vbuf);
        nk_buffer_init_default(&ebuf);
        nk_convert(&self->ctx, &dev->cmds, &vbuf, &ebuf, &config);

        /* setup vertex buffer pointer */
        {
        	const void *vertices = nk_buffer_memory_const(&vbuf);
	        glVertexPointer(2, GL_FLOAT, vs, (const void*)((const nk_byte*)vertices + vp));
	        glTexCoordPointer(2, GL_FLOAT, vs, (const void*)((const nk_byte*)vertices + vt));
	        glColorPointer(4, GL_UNSIGNED_BYTE, vs, (const void*)((const nk_byte*)vertices + vc));
    	}

        /* iterate over and execute each draw command */
        offset = (const nk_draw_index*)nk_buffer_memory_const(&ebuf);
        nk_draw_foreach(cmd, &self->ctx, &dev->cmds)
        {
            if (!cmd->elem_count) {
            	continue;
            }

            glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
            glScissor(
                (GLint)(cmd->clip_rect.x * scale.x),
                (GLint)((height - (GLint)(cmd->clip_rect.y + cmd->clip_rect.h)) * scale.y),
                (GLint)(cmd->clip_rect.w * scale.x),
                (GLint)(cmd->clip_rect.h * scale.y));
            glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, offset);
            offset += cmd->elem_count;
        }
        nk_clear(&self->ctx);
        nk_buffer_clear(&dev->cmds);
        nk_buffer_free(&vbuf);
        nk_buffer_free(&ebuf);
    }

    /* default OpenGL state */
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
}

struct NkInterface* nk_interface_gl2_init(
    struct nk_context** nk_context_out
) {
	struct NkInterface* iface = NULL;
	ctx_t* self = NULL;

	iface = (struct NkInterface*)malloc(sizeof(struct NkInterface));
	self = (ctx_t*)malloc(sizeof(ctx_t));

	if (!iface) {
		goto fail;
	}

	if (!self) {
		goto fail;
	}

	nk_init_default(&self->ctx, 0);
    self->ctx.clip.copy = NULL;//gl2_clipboard_copy;
    self->ctx.clip.paste = NULL;//gl2_clipboard_paste;
    self->ctx.clip.userdata = nk_handle_ptr(0);
    nk_buffer_init_default(&self->ogl.cmds);
    
    // we just use the deault font for now to make it easier
    internal_font_stash_begin(self);
    internal_font_stash_end(self);

    const struct NkInterface internal_iface = {
    	._private = self,
    	.quit = internal_quit,
    	.event = internal_event,
    	.render = internal_render
    };

    *iface = internal_iface;
    *nk_context_out = &self->ctx;

    return iface;

fail:
	if (iface) {
		free(iface);
		iface = NULL;
	}

	if (self) {
		free(self);
		self = NULL;
	}

	return NULL;
}

