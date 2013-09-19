//ToDo limit whistle to the real cursor, not ghost.

#include <math.h>

#include <allegro5\allegro.h>
#include <allegro5\allegro_font.h>
#include <allegro5\allegro_image.h>
#include <allegro5\allegro_primitives.h>

#include "const.h"
#include "functions.h"
#include "vars.h"

int main(){
	bool running=true;
	
	//Configurations outside of Allegro.
	srand(time(NULL));

	//Install Allegro and initialize modules.
	al_init();
	al_install_mouse();
	al_install_keyboard();
	al_init_image_addon();
	al_init_primitives_addon();

	//Event stuff.
	ALLEGRO_DISPLAY* display = al_create_display(600, 400);
	ALLEGRO_TIMER* timer = al_create_timer(1.0/30.0);

	ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
	al_register_event_source(queue, al_get_mouse_event_source());
	al_register_event_source(queue, al_get_keyboard_event_source());
	al_register_event_source(queue, al_get_display_event_source(display));
	al_register_event_source(queue, al_get_timer_event_source(timer));
	ALLEGRO_EVENT ev;

	//Other initial things.
	al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
	al_set_window_title(display, "Pikmin fangame engine");
	al_hide_mouse_cursor(display);
	if(smooth_scaling) al_set_new_bitmap_flags(ALLEGRO_MAG_LINEAR | ALLEGRO_MIN_LINEAR | ALLEGRO_MIPMAP);

	//Graphics.
	bmp_olimar = al_load_bitmap("Olimar.png");
	bmp_louie = al_load_bitmap("Louie.png");
	bmp_red = al_load_bitmap("Red.png");
	bmp_yellow = al_load_bitmap("Yellow.png");
	bmp_blue = al_load_bitmap("Blue.png");
	bmp_red_burrowed = al_load_bitmap("Red_burrowed.png");
	bmp_yellow_burrowed = al_load_bitmap("Yellow_burrowed.png");
	bmp_blue_burrowed = al_load_bitmap("Blue_burrowed.png");
	bmp_red_idle = al_load_bitmap("Red_idle.png");
	bmp_yellow_idle = al_load_bitmap("Yellow_idle.png");
	bmp_blue_idle = al_load_bitmap("Blue_idle.png");
	bmp_cursor = al_load_bitmap("Cursor.png");
	bmp_mouse_cursor = al_load_bitmap("Mouse_cursor.png");
	bmp_background = al_load_bitmap("Background.png");
	bmp_bubble = al_load_bitmap("Bubble.png");
	bmp_day_bubble = al_load_bitmap("Day_bubble.png");
	bmp_health_bubble = al_load_bitmap("Health_bubble.png");
	bmp_sun = al_load_bitmap("Sun.png");
	bmp_shadow = al_load_bitmap("Shadow.png");
	bmp_idle_glow = al_load_bitmap("Idle_glow.png");

	int font_ranges[] = {
		0x0020, 0x007F, //ASCII
		0x00A1, 0x00FF, //Latin 1
		0x0100, 0x017F, //Extended-A
		0x20AC, 0x20AC, //Euro
	};

	int font_area_name_ranges[] = {
		0x0020, 0x007F, //ASCII
	};

	ALLEGRO_BITMAP* temp_font_bitmap = al_load_bitmap("Font.png");  //We can't load the font directly because we want to set the ranges.
	if(temp_font_bitmap) font=al_grab_font_from_bitmap(temp_font_bitmap, 4, font_ranges);
	temp_font_bitmap = al_load_bitmap("Area_name_font.png");
	if(temp_font_bitmap) font_area_name=al_grab_font_from_bitmap(temp_font_bitmap, 1, font_area_name_ranges);
	al_destroy_bitmap(temp_font_bitmap);
	font_h = al_get_font_line_height(font);

	//Game content.
	load_game_content();

	//Initializing game things.
	sprays.clear();
	size_t n_spray_types = spray_types.size();
	for(size_t s=0; s<n_spray_types; s++){ sprays.push_back(0); }
	pikmin_in_onions.clear();
	size_t n_total_onions = pikmin_types.size();
	for(size_t o=0; o<n_total_onions; o++){ pikmin_in_onions.push_back(0); }

	//Some variables.
	sector s = sector();
	leaders.push_back(leader(0, 0, &s));
	leaders.back().main_color = al_map_rgb(255, 0, 0);
	leaders.back().health = 10;
	leaders.push_back(leader(300, 250, &s));
	leaders.back().main_color = al_map_rgb(0, 0, 255);
	leaders.back().health = 8;
	leaders.push_back(leader(350, 200, &s));
	leaders.back().main_color = al_map_rgb(0, 0, 255);
	leaders.back().health = 6;
	treasures.push_back(treasure(300, 100, 0, 40, 10, &s, 30, 50));
	pikmin_list.push_back(pikmin(&pikmin_types[0], 30, 30, &s));
	pikmin_list.push_back(pikmin(&pikmin_types[0], 40, 30, &s));
	pikmin_list.push_back(pikmin(&pikmin_types[1], 50, 30, &s));
	pikmin_list.push_back(pikmin(&pikmin_types[1], 60, 30, &s));
	pikmin_list.push_back(pikmin(&pikmin_types[2], 70, 30, &s));
	pikmin_list.push_back(pikmin(&pikmin_types[2], 80, 30, &s));
	pikmin_list.push_back(pikmin(&pikmin_types[0], 30, 200, &s));
	pikmin_list.back().burrowed=true;
	pikmin_list.push_back(pikmin(&pikmin_types[1], 50, 200, &s));
	pikmin_list.back().burrowed=true;
	pikmin_list.push_back(pikmin(&pikmin_types[2], 70, 200, &s));
	pikmin_list.back().burrowed=true;
	for(unsigned char p=0; p<10; p++){
		for(unsigned char t=0; t<3; t++){
			pikmin_list.push_back(pikmin(&pikmin_types[t], 100 + 10*p + 3*t, 30, &s));
		}
	}
	onions.push_back(onion(400, 100, &s, &pikmin_types[0]));
	onions.push_back(onion(400, 200, &s, &pikmin_types[1]));
	onions.push_back(onion(400, 300, &s, &pikmin_types[2]));
	sprays[0] = sprays[1] = 10;
	pikmin_in_onions[0] = 200;
	pikmin_in_onions[1] = 180;
	pikmin_in_onions[2] = 160;

	//Main loop.
	al_start_timer(timer);
	while(running){

		/*  ************************************************
		  *** | _ |                                  | _ | ***
		*****  \_/           EVENT HANDLING           \_/  *****
		  *** +---+                                  +---+ ***
		    ************************************************/

		al_wait_for_event(queue, &ev);
		if(ev.type==ALLEGRO_EVENT_DISPLAY_CLOSE){
			running=false;

		}else if(ev.type==ALLEGRO_EVENT_MOUSE_AXES){
			mouse_cursor_x = ev.mouse.x;
			mouse_cursor_y = ev.mouse.y;

			if(ev.mouse.dz!=0){
				//In Pikmin 2, at normal zoom and top view, a leader's height is roughly 6% that of the screen's. Max zoom, 12%. Min zoom, 3%.
				zoom_level += 0.1 * ev.mouse.dz;
				if(zoom_level > MAX_ZOOM_LEVEL) zoom_level = MAX_ZOOM_LEVEL;
				if(zoom_level < MIN_ZOOM_LEVEL) zoom_level = MIN_ZOOM_LEVEL;
			}

		}else if(ev.type==ALLEGRO_EVENT_MOUSE_BUTTON_DOWN){
			if(ev.mouse.button == 1){
				bool done = false;
				
				//First check if the leader should pluck a Pikmin.

				float closest_distance = 0;
				pikmin* closest_pikmin = NULL;
				size_t n_pikmin = pikmin_list.size();
				for(size_t p=0; p<n_pikmin; p++){
					if(!pikmin_list[p].burrowed) continue;
					float d = dist(leaders[current_leader].x, leaders[current_leader].y, pikmin_list[p].x, pikmin_list[p].y);
					if(closest_pikmin == NULL || d < closest_distance){
						closest_distance = d;
						closest_pikmin = &pikmin_list[p];
					}
				}

				if(closest_pikmin){
					if(closest_distance <= MIN_PLUCK_RANGE){
						//Pluck.
						closest_pikmin->burrowed = false;
						add_to_party(&leaders[current_leader], closest_pikmin);
						done = true;
					}
				}

				//Now check if the leader should open an onion's menu.

				if(!done){
					//ToDo
					size_t n_onions = onions.size();
					for(size_t o=0; o<n_onions; o++){
						if(dist(leaders[current_leader].x, leaders[current_leader].y, onions[o].x, onions[o].y) < MIN_ONION_CHECK_RANGE){
							//ToDo this is not how it works, there can be less onions on the field than the total number of Pikmin types.
							pikmin_in_onions[o]--;
							pikmin_list.push_back(pikmin(onions[o].type, onions[o].x, onions[o].y, onions[o].sec));
							add_to_party(&leaders[current_leader], &pikmin_list[pikmin_list.size()-1]);
							done = true;
						}
					}
				}
				
				//Now check if the leader should throw a Pikmin.

				if(!done){
					if(closest_party_member){
						leaders[current_leader].holding_pikmin = closest_party_member;
						done = true;
					}
				}

				//Now check if the leader should punch.

				if(!done){
					//ToDo
				}

			}else if(ev.mouse.button == 2){
				whistling = true;

			}

		}else if(ev.type==ALLEGRO_EVENT_MOUSE_BUTTON_UP){
			if(ev.mouse.button == 1){
				mob* holding_ptr = leaders[current_leader].holding_pikmin;
				if(holding_ptr){

					holding_ptr->x = leaders[current_leader].x;
					holding_ptr->y = leaders[current_leader].y;
					holding_ptr->z = leaders[current_leader].z;

					float d = dist(leaders[current_leader].x, leaders[current_leader].y, cursor_x, cursor_y);
					holding_ptr->speed_x = cos(leaders[current_leader].angle) * d * THROW_DISTANCE_MULTIPLIER;
					holding_ptr->speed_y = sin(leaders[current_leader].angle) * d * THROW_DISTANCE_MULTIPLIER;
					holding_ptr->speed_z = 2;

					holding_ptr->was_thrown = true;
				
					remove_from_party(&leaders[current_leader], holding_ptr);
					leaders[current_leader].holding_pikmin = NULL;
				}

			}else if(ev.mouse.button == 2){
				stop_whistling();
			}

		}else if(ev.type==ALLEGRO_EVENT_KEY_DOWN){
			//ToDo change to angles.
			if(ev.keyboard.keycode == ALLEGRO_KEY_W){
				leaders[current_leader].speed_y = -LEADER_MOVE_SPEED;

			}else if(ev.keyboard.keycode == ALLEGRO_KEY_A){
				leaders[current_leader].speed_x = -LEADER_MOVE_SPEED;

			}else if(ev.keyboard.keycode == ALLEGRO_KEY_S){
				leaders[current_leader].speed_y = LEADER_MOVE_SPEED;

			}else if(ev.keyboard.keycode == ALLEGRO_KEY_D){
				leaders[current_leader].speed_x = LEADER_MOVE_SPEED;

			}else if(ev.keyboard.keycode == ALLEGRO_KEY_TAB){
				leaders[current_leader].speed_x = 0;
				leaders[current_leader].speed_y = 0;

				size_t new_leader = (current_leader + 1) % leaders.size();
				bool swap_leaders = false;

				//Transfer all members onto the new leader, if the new leader is in the current one's party.
				size_t n_party_members = leaders[current_leader].party.size();
				for(size_t m=0; m<n_party_members; m++){
					if(leaders[current_leader].party[m] == &leaders[new_leader]) swap_leaders=true;
				}

				if(swap_leaders){
					for(size_t m=0; m<n_party_members; m++){
						mob* member = leaders[current_leader].party[0];
						remove_from_party(&leaders[current_leader], member);
						if(member != &leaders[new_leader]){
							add_to_party(&leaders[new_leader], member);
						}
					}

					add_to_party(&leaders[new_leader], &leaders[current_leader]);
				}

				current_leader = new_leader;

			}else if(ev.keyboard.keycode == ALLEGRO_KEY_LCTRL){
				leader* current_leader_ptr = &leaders[current_leader];

				size_t n_party_members = current_leader_ptr->party.size();
				for(size_t m=0; m<n_party_members; m++){
					remove_from_party(current_leader_ptr, current_leader_ptr->party[0]);
				}
			}else if(ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE){
				running = false;

			}else if(ev.keyboard.keycode == ALLEGRO_KEY_R){
				use_spray(0);

			}else if(ev.keyboard.keycode == ALLEGRO_KEY_F){
				use_spray(1);

			}else if(ev.keyboard.keycode == ALLEGRO_KEY_C){
				if(zoom_level < 1){
					zoom_level = MAX_ZOOM_LEVEL;
				}else if(zoom_level > 1){
					zoom_level = 1;
				}else{
					zoom_level = MIN_ZOOM_LEVEL;
				}

			}else if(ev.keyboard.keycode == ALLEGRO_KEY_T){
				//Debug testing.
				//ToDo remove.
				//leaders[current_leader].health--;
				//day_minutes += 60;
				if(whistling) zoom_level-=0.1;
				else zoom_level+=0.1;

			}

		}else if(ev.type==ALLEGRO_EVENT_KEY_UP){
			//ToDo change to angles.
			if(ev.keyboard.keycode == ALLEGRO_KEY_W || ev.keyboard.keycode==ALLEGRO_KEY_S){
				leaders[current_leader].speed_y = 0;
			}else if(ev.keyboard.keycode == ALLEGRO_KEY_A || ev.keyboard.keycode==ALLEGRO_KEY_D){
				leaders[current_leader].speed_x = 0;
			}

		}else if(ev.type==ALLEGRO_EVENT_TIMER && al_is_event_queue_empty(queue)){
			
			/*  ********************************************
			  ***  .-.                                .-.  ***
			***** ( L )          MAIN LOGIC          ( L ) *****
			  ***  `-�                                `-�  ***
				********************************************/

			
			/********************
			*              ***  *
			*   Whistle   * O * *
			*              ***  *
			*********************/

			if(whistling && whistle_radius < MAX_WHISTLE_RADIUS){
				whistle_radius += (1.0 / game_fps) * WHISTLE_RADIUS_GROWTH_PS;
				if(whistle_radius > MAX_WHISTLE_RADIUS){
					whistle_radius = MAX_WHISTLE_RADIUS;
					whistle_max_hold = WHISTLE_MAX_HOLD_TIME;
				}
			}

			if(whistle_max_hold > 0){
				whistle_max_hold -= 1.0/game_fps;
				if(whistle_max_hold <= 0){
					stop_whistling();
				}
			}


			/********************
			*             /\  *
			*   Pikmin   (@:) *
			*             \/  *
			*********************/

			size_t n_treasures = treasures.size();
			size_t n_pikmin = pikmin_list.size();
			for(size_t p=0; p<n_pikmin; p++){
				pikmin* pik_ptr = &pikmin_list[p];
				if(whistling){
					if(
						dist(pik_ptr->x, pik_ptr->y, cursor_x, cursor_y) <= whistle_radius &&
						pik_ptr->following_party==NULL &&
						!pik_ptr->burrowed &&
						!pik_ptr->was_thrown){

							//Pikmin got whistled.
							add_to_party(&leaders[current_leader], pik_ptr);
							pik_ptr->enemy_attacking = NULL;

							//ToDo optimize this instead of running through the spot vector.
							if(pik_ptr->carrying_treasure){
								for(size_t s=0; s<pik_ptr->carrying_treasure->max_carriers; s++){
									if(pik_ptr->carrying_treasure->carrier_spots[s] == pik_ptr){
										pik_ptr->carrying_treasure->carrier_spots[s] = NULL;
									}
								}
							}

							pik_ptr->carrying_treasure = NULL;
							
					}
				}

				if(pik_ptr->following_party){
					pik_ptr->target_x = pik_ptr->following_party->x + random(0, 30);
					pik_ptr->target_y = pik_ptr->following_party->y + random(0, 30);
				}

				//Finding tasks.
				if(
					!pik_ptr->following_party &&
					!pik_ptr->carrying_treasure &&
					!pik_ptr->enemy_attacking &&
					!pik_ptr->burrowed
					){
						for(size_t t=0; t<n_treasures; t++){
							if(dist(pik_ptr->x, pik_ptr->y, treasures[t].x, treasures[t].y)<=pik_ptr->size + treasures[t].size + MIN_PIKMIN_TASK_RANGE){
								//ToDo don't take the treasure if all spots are taken already.
								pik_ptr->carrying_treasure = &treasures[t];
								//ToDo remove this random cycle and replace with something more optimal.
								bool valid_spot = false;
								unsigned int spot = 0;
								while(!valid_spot){
									spot = random(0, treasures[t].max_carriers - 1);
									valid_spot = !treasures[t].carrier_spots[spot];
								}
								treasures[t].carrier_spots[spot] = pik_ptr;

								float angle = ((M_PI * 2) / treasures[t].max_carriers) * spot;
								pik_ptr->target_x = treasures[t].x + cos(angle) * (treasures[t].size + pik_ptr->size / 2);
								pik_ptr->target_y = treasures[t].y + sin(angle) * (treasures[t].size + pik_ptr->size / 2);
								pik_ptr->go_to_target = true;

								break;
							}
						}
				}
				
				pik_ptr->tick();

			}


			/********************
			*              .-.  *
			*   Leaders   (*:O) *
			*              `-�  *
			*********************/

			if(leaders[current_leader].holding_pikmin){
				leaders[current_leader].holding_pikmin->x = leaders[current_leader].x+8;
				leaders[current_leader].holding_pikmin->y = leaders[current_leader].y;
			}

			size_t n_leaders = leaders.size();
			for(size_t l=0; l<n_leaders; l++){
				if(whistling){
					if(l != current_leader){
						if(
							dist(leaders[l].x, leaders[l].y, cursor_x, cursor_y) <= whistle_radius &&
							!leaders[l].following_party &&
							!leaders[l].was_thrown){
							//Leader got whistled.
							add_to_party(&leaders[current_leader], &leaders[l]);

							size_t n_party_members = leaders[l].party.size();
							for(size_t m=0; m<n_party_members; m++){
								mob* member = leaders[l].party[0];
								remove_from_party(&leaders[l], member);
								add_to_party(&leaders[current_leader], member);
							}
						}
					}
				}

				if(leaders[l].following_party){
					leaders[l].target_x = leaders[l].following_party->x;
					leaders[l].target_y = leaders[l].following_party->y + 30;
				}
				
				leaders[l].tick();
			}


			/******************
			*            ***  *
			*   Party   ****# *
			*            ***  *
			*******************/

			float closest_distance = 0;
			size_t n_members = leaders[current_leader].party.size();
			closest_party_member = NULL;

			if(n_members > 0){

				for(size_t m=0; m<n_members; m++){
					mob* ccc=leaders[current_leader].party[m];
					float xxx=leaders[current_leader].party[m]->x;
					float yyy=leaders[current_leader].party[m]->y;
					float d = dist(leaders[current_leader].x, leaders[current_leader].y, leaders[current_leader].party[m]->x, leaders[current_leader].party[m]->y);
					if(m==0 || d < closest_distance){
						closest_distance = d;
						closest_party_member = leaders[current_leader].party[m];
					}
				}

				if(closest_distance > MIN_PIKMIN_GRABBING_RANGE){
					closest_party_member = NULL;
				}
			}


			/*************************
			*                   .-.  *
			*   Timer things   ( L ) *
			*                   `-�  *
			**************************/

			day_minutes += (1.0f / game_fps) * day_minutes_per_irl_sec;
			if(day_minutes > 60 * 24) day_minutes -= 60 * 24;

			idle_glow_angle+=(1.0 / game_fps) * (IDLE_GLOW_SPIN_SPEED);


			/********************
			*             .-.   *
			*   Cursor   ( = )> *
			*             `-�   *
			*********************/

			float mcx = mouse_cursor_x, mcy = mouse_cursor_y;
			ALLEGRO_TRANSFORM screen_to_world_transform = get_world_to_screen_transform();
			al_invert_transform(&screen_to_world_transform);
			al_transform_coordinates(&screen_to_world_transform, &mcx, &mcy);
			cursor_x = mcx;
			cursor_y = mcy;

			leaders[current_leader].angle = atan2(cursor_y - leaders[current_leader].y, cursor_x - leaders[current_leader].x);
			if(dist(leaders[current_leader].x, leaders[current_leader].y, cursor_x, cursor_y) > CURSOR_MAX_DIST){
				//Cursor goes beyond the range limit.
				cursor_x = leaders[current_leader].x + (cos(leaders[current_leader].angle) * CURSOR_MAX_DIST);
				cursor_y = leaders[current_leader].y + (sin(leaders[current_leader].angle) * CURSOR_MAX_DIST);
			}

			/**********************
			*                 *   *
			*   Particles   *   * *
			*                ***  *
			***********************/

			//Tick all particles.
			size_t n_particles = particles.size();
			for(size_t p=0; p<n_particles; ){
				if(!particles[p].tick()){
					particles.erase(particles.begin() + p);
					n_particles--;
				}else{
					p++;
				}
			}

			//ToDo the particles can't be created once per frame! That's overkill! ...right?
			for(size_t l = 0; l < n_leaders; l++){
				if(leaders[l].was_thrown)
					random_particle_fire(leaders[l].x, leaders[l].y, 1, 1, 0.3, 0.5, 3, 4, change_alpha(leaders[l].main_color, 192));
			}

			for(size_t p = 0; p < n_pikmin; p++){
				if(pikmin_list[p].was_thrown)
					random_particle_fire(pikmin_list[p].x, pikmin_list[p].y, 1, 1, 0.3, 0.5, 3, 4, change_alpha(pikmin_list[p].main_color, 192));
			}
			



			/*  ***************************************
			  *** |  |                           |  | ***
			***** |__|          DRAWING          |__| *****
			  ***  \/                             \/  ***
				***************************************/

			al_clear_to_color(al_map_rgb(0,0,0));

			ALLEGRO_TRANSFORM normal_transform;
			al_identity_transform(&normal_transform);
			
			ALLEGRO_TRANSFORM world_to_screen_transform = get_world_to_screen_transform();
			al_use_transform(&world_to_screen_transform);

			/* Layer 1
			*************************
			*                ^^^^^^ *
			*   Background   ^^^^^^ *
			*                ^^^^^^ *
			*************************/

			al_draw_scaled_bitmap(bmp_background, 0, 0, 512, 512, scr_w/2 - 512, scr_h/2 - 512, 512, 512, 0);
			al_draw_scaled_bitmap(bmp_background, 0, 0, 512, 512, scr_w/2 - 512, scr_h/2, 512, 512, 0);
			al_draw_scaled_bitmap(bmp_background, 0, 0, 512, 512, scr_w/2, scr_h/2 - 512, 512, 512, 0);
			al_draw_scaled_bitmap(bmp_background, 0, 0, 512, 512, scr_w/2, scr_h/2, 512, 512, 0);


			/* Layer 2
			************************
			*                  ##  *
			*   Mob shadows   #### *
			*                  ##  *
			************************/

			float shadow_stretch = 0;
			
			if(day_minutes < 60*5 || day_minutes > 60*20){
				shadow_stretch = 1;
			}else if(day_minutes < 60*12){
				shadow_stretch = 1-((day_minutes - 60*5) / (60*12 - 60*5));
			}else{
				shadow_stretch = (day_minutes - 60*12) / (60*20 - 60*12);
			}

			n_leaders = leaders.size();
			for(size_t l = 0; l < n_leaders; l++){
				draw_shadow(leaders[l].x, leaders[l].y, 32, leaders[l].z - leaders[l].sec->floors[0].z, shadow_stretch);
			}

			n_pikmin = pikmin_list.size();
			for(size_t p = 0; p < n_pikmin; p++){
				draw_shadow(pikmin_list[p].x, pikmin_list[p].y, 18, pikmin_list[p].z - pikmin_list[p].sec->floors[0].z, shadow_stretch);
			}


			/* Layer 3
			*********************
			*             .-.   *
			*   Cursor   ( = )> *
			*             `-�   *
			*********************/

			al_use_transform(&normal_transform);
			al_draw_rotated_bitmap(bmp_mouse_cursor, 48, 48, mouse_cursor_x, mouse_cursor_y, leaders[current_leader].angle, 0);
			al_use_transform(&world_to_screen_transform);
			al_draw_scaled_rotated_bitmap(bmp_cursor, 48, 48, cursor_x, cursor_y, 0.5, 0.5, leaders[current_leader].angle, 0);

			
			/* Layer 4
			****************
			*          \o/ *
			*   Mobs    Y  *
			*          / \ *
			****************/

			//Pikmin
			n_pikmin = pikmin_list.size();
			for(size_t p = 0; p<n_pikmin; p++){
				ALLEGRO_BITMAP* bm;
				if(pikmin_list[p].type->name=="R"){
					if(pikmin_list[p].burrowed) bm=bmp_red_burrowed; else if(!pikmin_list[p].following_party) bm=bmp_red_idle; else bm=bmp_red;
				}else if(pikmin_list[p].type->name=="Y"){
					if(pikmin_list[p].burrowed) bm=bmp_yellow_burrowed; else if(!pikmin_list[p].following_party) bm=bmp_yellow_idle; else  bm=bmp_yellow;
				}if(pikmin_list[p].type->name=="B"){
					if(pikmin_list[p].burrowed) bm=bmp_blue_burrowed; else if(!pikmin_list[p].following_party) bm=bmp_blue_idle; else  bm=bmp_blue;
				}
				al_draw_scaled_rotated_bitmap(bm, 18, 18, pikmin_list[p].x, pikmin_list[p].y, 0.5, 0.5, pikmin_list[p].angle, 0); //ToDo actual coordinates

				if(!pikmin_list[p].following_party){
					al_draw_tinted_scaled_rotated_bitmap(
						bmp_idle_glow,
						change_alpha(pikmin_list[p].main_color, 128),
						18,
						18,
						pikmin_list[p].x,
						pikmin_list[p].y,
						0.5,
						0.5,
						idle_glow_angle,
						0
						);
					//ToDo actual coordinates.
				}
			}

			//Leaders.
			for(size_t l=0; l<n_leaders; l++){
				ALLEGRO_BITMAP* bm = (l==0) ? bmp_olimar : bmp_louie;
				al_draw_scaled_rotated_bitmap(bm, 32, 32, leaders[l].x, leaders[l].y, 0.5, 0.5, leaders[l].angle, 0);
			}

			if(whistle_radius > 0){
				al_draw_circle(cursor_x, cursor_y, whistle_radius, al_map_rgb(192, 192, 0), 2);
			}

			//Onions.
			size_t n_onions = onions.size();
			for(size_t o=0; o<n_onions; o++){
				al_draw_filled_circle(onions[o].x, onions[o].y, onions[o].size, change_alpha(onions[o].type->color, 128));
			}

			//Treasures.
			for(size_t t=0; t<n_treasures; t++){
				al_draw_filled_circle(treasures[t].x, treasures[t].y, treasures[t].size, al_map_rgb(128, 255, 255));
			}


			/* Layer 5
			***********************
			*                 *   *
			*   Particles   *   * *
			*                ***  *
			***********************/

			if(particle_quality>0){
				n_particles = particles.size();
				for(size_t p=0; p<n_particles; p++){
					if(particle_quality==1){
						al_draw_filled_rectangle(
							particles[p].x - particles[p].size*0.5,
							particles[p].y - particles[p].size*0.5,
							particles[p].x + particles[p].size*0.5,
							particles[p].y + particles[p].size*0.5,
							change_alpha(particles[p].color, (particles[p].time / particles[p].starting_time) * particles[p].color.a * 255)
							);
					}else{
						al_draw_filled_circle(
							particles[p].x,
							particles[p].y,
							particles[p].size * 0.5,
							change_alpha(particles[p].color, (particles[p].time / particles[p].starting_time) * particles[p].color.a * 255)
							);
					}
				}
			}


			/* Layer 6
			*************************
			*              --==## *
			*   Daylight   --==## *
			*              --==## *
			*************************/

			al_use_transform(&normal_transform);

			if(daylight_effect){
				al_draw_filled_rectangle(0, 0, scr_w, scr_h, get_daylight_color());
			}


			/* Layer 7
			*****************
			*           (1) *
			*   HUD         *
			*         1/2/3 *
			*****************/
			
			//ToDo actual image coordinates (remember the health bubble has a different image than the regular ones)
			//ToDo actual leader top healths.
			int leader_img_size = 32;
			int bubble_img_size = 64;

			//Leader health.
			for(size_t l = 0; l < 3; l++){
				if(n_leaders < l + 1) continue;

				size_t l_nr = (current_leader+l)%n_leaders;
				ALLEGRO_BITMAP* bm = (l_nr==0) ? bmp_olimar : bmp_louie;

				int icons_size;
				if(l == 0) icons_size = 32; else icons_size = 20;

				int y_offset;
				if(l == 0) y_offset = 0; else if(l == 1) y_offset = 44; else y_offset = 80;

				al_draw_scaled_bitmap(
					bm, 0, 0, leader_img_size, leader_img_size,
					32 - icons_size / 2,
					scr_h - (32 + y_offset + icons_size / 2),
					icons_size, icons_size, 0
					);
				al_draw_scaled_bitmap(bmp_bubble, 0, 0, bubble_img_size, bubble_img_size,
					32 - (icons_size * 1.6) / 2,
					scr_h - (32 + y_offset + (icons_size * 1.6) / 2),
					icons_size * 1.6, icons_size * 1.6, 0
					);

				draw_health(
					32 + icons_size * 1.5,
					scr_h - (32 + y_offset),
					leaders[l_nr].health, 10,
					icons_size / 2, true);
				al_draw_scaled_bitmap(
					bmp_health_bubble, 0, 0, bubble_img_size, bubble_img_size,
					(32 + icons_size * 1.5) - (icons_size * 1.2) / 2,
					scr_h - (32 + y_offset + (icons_size * 1.2) / 2),
					icons_size * 1.2, icons_size * 1.2, 0);
			}

			//Day hour.
			al_draw_text(font, al_map_rgb(255, 255, 255), 8, 8, 0,
				(to_string((long long) (day_minutes/60)) + ":" + to_string((long long) ((int) (day_minutes)%60))).c_str());

			//Sun Meter.
			unsigned char n_hours = (day_minutes_end - day_minutes_start) / 60;
			unsigned int sun_meter_span = (scr_w - 100) - 20;
			unsigned short interval = sun_meter_span / n_hours;
			for(unsigned char h = 0; h<n_hours + 1; h++){
				al_draw_scaled_bitmap(bmp_bubble, 0, 0, 64, 64, 20 + h*interval, 40, 10, 10, 0);
			}

			float day_passed_ratio = (float) (day_minutes - day_minutes_start) / (float) (day_minutes_end - day_minutes_start);
			al_draw_scaled_bitmap(bmp_sun, 0, 0, 64, 64, 10 + day_passed_ratio * sun_meter_span, 30, 30, 30, 0);

			//Pikmin count.
			//Count how many Pikmin only.
			n_leaders = leaders.size();
			size_t pikmin_in_party = leaders[current_leader].party.size();
			for(size_t l=0; l<n_leaders; l++){
				//If this leader is following the current one, then he's not a Pikmin, subtract him from the party count total.
				if(leaders[l].following_party == &leaders[current_leader]) pikmin_in_party--;
			}

			//Closest party member.
			if(closest_party_member){
				if(typeid(*closest_party_member) == typeid(pikmin)){
					pikmin* pikmin_ptr = dynamic_cast<pikmin*>(closest_party_member);
					ALLEGRO_BITMAP* bm;
					if(pikmin_ptr->type->name=="R") bm=bmp_red;
					else if(pikmin_ptr->type->name=="Y") bm=bmp_yellow;
					if(pikmin_ptr->type->name=="B") bm=bmp_blue;
					
					al_draw_bitmap(bm, 250, scr_h - 20 - font_h, 0);
				}
			}

			al_draw_scaled_bitmap(bmp_bubble, 0, 0, 64, 64, 240, scr_h - 30 - font_h, 40, 40, 0);

			//Pikmin count numbers.
			al_draw_text(
				font, al_map_rgb(255, 255, 255), scr_w - 20, scr_h - 20 - font_h, ALLEGRO_ALIGN_RIGHT,
				(to_string((long long) pikmin_in_party) + "/" + to_string((long long) pikmin_list.size()) + "/1234").c_str()
				);

			//Day number.
			al_draw_text(
				font, al_map_rgb(255, 255, 255), scr_w - 50, 40, ALLEGRO_ALIGN_CENTER,
				(to_string((long long) day)).c_str());

			al_draw_scaled_bitmap(bmp_day_bubble, 0, 0, 128, 152, scr_w - 80, 10, 60, 70, 0);

			//Sprays.
			//ToDo 1 spray, and more than 2 sprays.
			if(n_spray_types < 3){
				al_draw_filled_circle(32, scr_h / 2 - 16, 8, spray_types[0].main_color);
				al_draw_text(font, al_map_rgb(255, 255, 255), 48, scr_h / 2 - 16 - font_h / 2, 0, to_string((long long) sprays[0]).c_str());
				al_draw_filled_circle(32, scr_h / 2 + 16, 8, spray_types[1].main_color);
				al_draw_text(font, al_map_rgb(255, 255, 255), 48, scr_h / 2 + 16 - font_h / 2, 0, to_string((long long) sprays[1]).c_str());
			}

			//Fractions.
			for(size_t t=0; t<n_treasures; t++){
				//ToDo only show if it has at least one carrier.
				//ToDo fraction color.
				//ToDo it's not taking Pikmin strength into account.
				//ToDo can't this be more optimized than running through the vector of carrier spots?
				size_t n_pikmin_on_treasure = 0;
				for(size_t p=0; p<treasures[t].max_carriers; p++) if(treasures[t].carrier_spots[p]) n_pikmin_on_treasure++;
				if(n_pikmin_on_treasure>0)
					draw_fraction(treasures[t].x, treasures[t].y, n_pikmin_on_treasure, treasures[t].weight, al_map_rgb(128, 128, 128));
			}

			//ToDo remove me al_draw_text(font_area_name, al_map_rgb(224, 232, 64), scr_w/2, scr_h/2, ALLEGRO_ALIGN_CENTER, "Green Hill Zone");
			
			al_flip_display();
		}
	}
}
